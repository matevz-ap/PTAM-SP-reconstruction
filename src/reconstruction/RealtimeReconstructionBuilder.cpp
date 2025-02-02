#include "RealtimeReconstructionBuilder.h"
#include <algorithm>

#include <theia/util/filesystem.h>
#include <theia/image/image.h>
#include <theia/image/descriptor/sift_descriptor.h>
#include <theia/matching/create_feature_matcher.h>
#include <theia/matching/image_pair_match.h>
#include <theia/matching/feature_matcher_utils.h>
#include <theia/sfm/colorize_reconstruction.h>
#include <theia/sfm/reconstruction_estimator_utils.h>
#include <theia/sfm/estimators/feature_correspondence_2d_3d.h>
#include <theia/io/write_ply_file.h>
#include <theia/sfm/camera/camera.h>
#include <colmap/retrieval/utils.h>

#include <filesystem>
namespace fs = std::filesystem;

RealtimeReconstructionBuilder::RealtimeReconstructionBuilder(const Options& options)
        : options_(options) {

    // Initialize descriptor extractor
    descriptor_extractor_ = std::make_unique<SiftGpuDescriptorExtractor>(options_.descriptor_extractor_options);

    // Initialize image retrieval
    image_retrieval_ = std::make_unique<ImageRetrieval>(options_.image_retrieval_options);

    // Initialize matcher
    feature_matcher_ = std::make_unique<RealtimeFeatureMatcher>(options_.matching_options,
                                                                options_.intrinsics_prior);

    // Initialize SfM objects
    view_graph_ = std::make_unique<theia::ViewGraph>();
    reconstruction_ = std::make_unique<theia::Reconstruction>();
    reconstruction_estimator_.reset(theia::ReconstructionEstimator::Create(options_.reconstruction_estimator_options));
}

bool RealtimeReconstructionBuilder::InitializeReconstruction(
        const std::string& image1_fullpath,
        const std::string& image2_fullpath) {

    // Check if initialized
    if (IsInitialized()) {
        reconstruction_message_ = "Initialize error: Reconstruction is already initialized.";
        return false;
    }

    // Read the images
    if (!theia::FileExists(image1_fullpath)) {
        reconstruction_message_ = "Initialize error: Image " + image1_fullpath + " does not exist\n";
        return false;
    }
    std::string image1_filename;
    theia::GetFilenameFromFilepath(image1_fullpath, true, &image1_filename);
    theia::FloatImage image1(image1_fullpath);

    if (!theia::FileExists(image2_fullpath)) {
        reconstruction_message_ = "Initialize error: Image " + image2_fullpath + " does not exist\n";
        return false;
    }
    std::string image2_filename;
    theia::GetFilenameFromFilepath(image2_fullpath, true, &image2_filename);
    theia::FloatImage image2(image2_fullpath);

    // Add new views to reconstruction and set intrinsics priors
    theia::ViewId view1_id = reconstruction_->AddView(image1_filename, 0);
    theia::View* view1 = reconstruction_->MutableView(view1_id);
    // Eigen::Matrix<double, 3, 4> projection_matrix1;
    // projection_matrix1 << 2607.429996, -3.844898, 1498.178098, -533936.661373,
    //                     -192.076910, 2862.552532, 681.798177, 23434.686572,
    //                     -0.241605, -0.030951, 0.969881, 22.540121;
    // theia::Camera camera1;
    // camera1.InitializeFromProjectionMatrix(1600, 1200, projection_matrix1);
    *(view1->MutableCameraIntrinsicsPrior()) = options_.intrinsics_prior;
    // *(view1->MutableCameraIntrinsicsPrior()) = camera1.CameraIntrinsicsPriorFromIntrinsics();

    theia::ViewId view2_id = reconstruction_->AddView(image2_filename, 0);
    theia::View* view2 = reconstruction_->MutableView(view2_id);
    // Eigen::Matrix<double, 3, 4> projection_matrix2;
    // projection_matrix2 << 1977.784758, -1210.002933, 1915.072827, -784204.529312,
    //                     976.333235, 2626.930526, 917.583083, -178022.203917,
    //                     -0.416183, 0.073779, 0.906283, 71.276252;
    // theia::Camera camera2;
    // camera2.InitializeFromProjectionMatrix(1600, 1200, projection_matrix2);
    *(view2->MutableCameraIntrinsicsPrior()) = options_.intrinsics_prior;
    // *(view2->MutableCameraIntrinsicsPrior()) = camera2.CameraIntrinsicsPriorFromIntrinsics();

    // Feature extraction
    std::vector<theia::Keypoint> image1_keypoints;
    std::vector<Eigen::VectorXf> image1_descriptors;
    descriptor_extractor_->DetectAndExtractDescriptors(image1, &image1_keypoints, &image1_descriptors);

    std::vector<theia::Keypoint> image2_keypoints;
    std::vector<Eigen::VectorXf> image2_descriptors;
    descriptor_extractor_->DetectAndExtractDescriptors(image2, &image2_keypoints, &image2_descriptors);

    // Feature matching
    feature_matcher_->AddImage(image1_filename, image1_keypoints, image1_descriptors);
    feature_matcher_->AddImage(image2_filename, image2_keypoints, image2_descriptors);

    // Add to image retrieval
    image_retrieval_->AddImage(view1_id, image1_keypoints, image1_descriptors);
    image_retrieval_->AddImage(view2_id, image2_keypoints, image2_descriptors);

    std::vector<theia::ImagePairMatch> matches;
    std::vector<std::pair<std::string, std::string>> pairs_to_match;
    pairs_to_match.emplace_back(std::make_pair(image1_filename, image2_filename));
    feature_matcher_->MatchImages(&matches, pairs_to_match);

    // Add matches to view graph
    if (!matches.empty()) {
        theia::ImagePairMatch match = matches.front();
        view_graph_->AddEdge(view1_id, view2_id, match.twoview_info);

        // Add tracks to reconstruction
        for (const auto& correspondence : match.correspondences) {
            const auto image1_feature = std::make_pair(view1_id, correspondence.feature1);
            const auto image2_feature = std::make_pair(view2_id, correspondence.feature2);

            // Build track
            std::vector<std::pair<theia::ViewId, theia::Feature>> track;
            track.emplace_back(image1_feature);
            track.emplace_back(image2_feature);

            theia::TrackId track_id = reconstruction_->AddTrack(track);
        }
    } else {
        reconstruction_message_ = "Initialize error: No matches found.";
        ResetReconstruction();
        return false;
    }

    // Build reconstruction
    reconstruction_estimator_->Estimate(view_graph_.get(), reconstruction_.get());

    // Check if both views were estimated successfully
    if (reconstruction_->NumViews() != NumEstimatedViews(*reconstruction_)) {
        reconstruction_message_ = "Initialize error: Views were not estimated.";
        ResetReconstruction();
        return false;
    }

    return true;
}

bool RealtimeReconstructionBuilder::ExtendReconstruction(const std::string& image_fullpath) {

    // Check if initialized
    if (!IsInitialized()) {
        reconstruction_message_ = "Extend error: Reconstruction is not initialized.";
        return false;
    }

    // Read the image
    if (!theia::FileExists(image_fullpath)) {
        reconstruction_message_ = "Extend error: Image " + image_fullpath + " does not exist\n";
        return false;
    }
    std::string image_filename;
    theia::GetFilenameFromFilepath(image_fullpath, true, &image_filename);
    theia::FloatImage image(image_fullpath);

    // Add new view to reconstruction and set intrinsics prior
    theia::ViewId view_id = reconstruction_->AddView(image_filename, 0);
    theia::View* view = reconstruction_->MutableView(view_id);
    // Eigen::Matrix<double, 3, 4> projection_matrix;
    // projection_matrix << 805.625978, -1995.776821, 2100.258376, -861180.861253,
    //                     1844.712809, 1881.939141, 1323.362766, -479716.738619,
    //                     -0.509301, 0.248881, 0.823814, 129.344349;
    // theia::Camera camera;
    // camera.InitializeFromProjectionMatrix(1600, 1200, projection_matrix);
    *(view->MutableCameraIntrinsicsPrior()) = options_.intrinsics_prior;
    // *(view->MutableCameraIntrinsicsPrior()) = camera.CameraIntrinsicsPriorFromIntrinsics();

    // Feature extraction
    std::vector<theia::Keypoint> image_keypoints;
    std::vector<Eigen::VectorXf> image_descriptors;
    descriptor_extractor_->DetectAndExtractDescriptors(image, &image_keypoints, &image_descriptors);

    // Image retrieval (set pairs to match)
    std::vector<colmap::retrieval::ImageScore> image_scores =
            image_retrieval_->QueryImage(image_keypoints, image_descriptors);

    std::vector<std::pair<std::string, std::string>> pairs_to_match;
    for (const auto& image_score : image_scores) {
        auto other_view_id = static_cast<theia::ViewId>(image_score.image_id);
        std::string other_filename = reconstruction_->View(other_view_id)->Name();
        pairs_to_match.emplace_back(std::make_pair(other_filename, image_filename));
    }

    // Feature matching
    feature_matcher_->AddImage(image_filename, image_keypoints, image_descriptors);
    std::vector<theia::ImagePairMatch> matches;
    feature_matcher_->MatchImages(&matches, pairs_to_match);

    // Add to image retrieval
    image_retrieval_->AddImage(view_id, image_keypoints, image_descriptors);

    // Add matches to view graph
    if (!matches.empty()) {

        for (const auto& match : matches) {
            theia::ViewId view1_id = reconstruction_->ViewIdFromName(match.image1);
            theia::ViewId view2_id = reconstruction_->ViewIdFromName(match.image2);
            assert(view1_id < view2_id);
            assert(view_id == view2_id);
            view_graph_->AddEdge(view1_id, view2_id, match.twoview_info);

            // Add tracks and observations to reconstruction
            for (const auto& correspondence : match.correspondences) {

                // Check if correspondence is already reconstructed
                const theia::View* view1 = reconstruction_->View(view1_id);
                const theia::TrackId* track_id_ptr = view1->GetTrackId(correspondence.feature1);
                if (track_id_ptr == nullptr) {

                    // Add new track to reconstruction
                    std::vector<std::pair<theia::ViewId, theia::Feature>> new_track;
                    new_track.emplace_back(std::make_pair(view1_id, correspondence.feature1));
                    new_track.emplace_back(std::make_pair(view2_id, correspondence.feature2));
                    reconstruction_->AddTrack(new_track);
                } else {

                    // Observation of the track may be already added from the previous match
                    theia::TrackId existing_track_id = *track_id_ptr;
                    const theia::View* view2 = reconstruction_->View(view2_id);
                    if (view2->GetFeature(existing_track_id) == nullptr) {

                        // Add observation of existing track
                        reconstruction_->AddObservation(view2_id, existing_track_id, correspondence.feature2);
                    }
                }
            }
        }
    } else {
        reconstruction_message_ = "Extend error: No matches found.";
        return false;
    }

    // Build reconstruction
    auto summary = reconstruction_estimator_->Estimate(view_graph_.get(), reconstruction_.get());

    // Check if view was added successfully
    if (reconstruction_->NumViews() != NumEstimatedViews(*reconstruction_)) {
        reconstruction_message_ = "Extend error: View could not be estimated.";
        return false;
    }

    return true;
}

bool RealtimeReconstructionBuilder::RemoveView(theia::ViewId view_id) {
    const theia::View* view = reconstruction_->View(view_id);
    if (view != nullptr) {
        bool success;

        // Remove from matcher
        feature_matcher_->RemoveImage(reconstruction_->View(view_id)->Name());

        // Remove view from view_graph
        success = view_graph_->RemoveView(view_id);
        if (!success) return false;

        // Remove from reconstruction
        success = reconstruction_->RemoveView(view_id);
        if (!success) return false;

        // Reestimate tracks
        reconstruction_estimator_->Estimate(view_graph_.get(), reconstruction_.get());
        return true;
    } else {
        reconstruction_message_ = "Remove view error: View id " + std::to_string(view_id) + " does not exist.";
        return false;
    }
}

bool RealtimeReconstructionBuilder::RemoveUnestimatedViews() {
    bool success = true;
    for (const auto& view_id : reconstruction_->ViewIds()) {
        const class theia::View* view = reconstruction_->View(view_id);
        if (!view->IsEstimated()) {
            bool result = RemoveView(view_id);
            success = success & result;
        }
    }
    return success;
}

bool RealtimeReconstructionBuilder::ResetReconstruction() {
    bool success = true;
    for (const auto& view_id : reconstruction_->ViewIds()) {
        bool result = RemoveView(view_id);
        success = success & result;
    }
    return success;
}

bool RealtimeReconstructionBuilder::LocalizeImage(const theia::FloatImage& image,
                                                  theia::CalibratedAbsolutePose& pose) {

    if (image_retrieval_->GetNumImages() < 1) {
        return false;
    }

    // Feature extraction
    std::vector<theia::Keypoint> image_keypoints;
    std::vector<Eigen::VectorXf> image_descriptors;
    descriptor_extractor_->DetectAndExtractDescriptors(image, &image_keypoints, &image_descriptors);

    // Image retrieval
    std::vector<colmap::retrieval::ImageScore> image_scores =
            image_retrieval_->QueryImage(image_keypoints, image_descriptors);

    std::vector<theia::ViewId> views_to_match;
    for (const auto& image_score : image_scores) {
        auto view_id = static_cast<theia::ViewId>(image_score.image_id);
        if (reconstruction_->View(view_id)->IsEstimated()) {
            views_to_match.push_back(view_id);
        }
    }

    // Call localization
    bool success = LocalizeImage(image_keypoints, image_descriptors, views_to_match, pose);
    return success;
}

bool RealtimeReconstructionBuilder::LocalizeImage(const theia::FloatImage& image,
                                                  const theia::CalibratedAbsolutePose& prev_pose,
                                                  theia::CalibratedAbsolutePose& pose) {

    // Compute distances to estimated views
    std::vector<std::pair<theia::ViewId, double>> distances;
    std::unordered_set<theia::ViewId> view_candidates;
    GetEstimatedViewsFromReconstruction(*reconstruction_, &view_candidates);
    distances.reserve(view_candidates.size());

    if (view_candidates.empty()) {
        return false;
    }

    for (const auto& view_id : view_candidates) {
        const theia::View* view = reconstruction_->View(view_id);
        double distance = (prev_pose.position - view->Camera().GetPosition()).norm();
        distances.emplace_back(std::make_pair(view_id, distance));
    }

    // Get view with shortest distance
    std::pair<theia::ViewId, double> min_distance = distances[0];
    for (const auto& distance : distances) {
        if (distance.second < min_distance.second) {
            min_distance = distance;
        }
    }

    // Call localization
    std::vector<theia::Keypoint> image_keypoints;
    std::vector<Eigen::VectorXf> image_descriptors;
    descriptor_extractor_->DetectAndExtractDescriptors(image, &image_keypoints, &image_descriptors);

    std::vector<theia::ViewId> views_to_match = {min_distance.first};
    bool success = LocalizeImage(image_keypoints, image_descriptors, views_to_match, pose);
    return success;
}

bool RealtimeReconstructionBuilder::LocalizeImage(const std::vector<theia::Keypoint>& image_keypoints,
                                                  const std::vector<Eigen::VectorXf>& image_descriptors,
                                                  const std::vector<theia::ViewId>& views_to_match,
                                                  theia::CalibratedAbsolutePose& pose) {
    if (views_to_match.empty()) {
        return false;
    }

    // Feature extraction
    theia::KeypointsAndDescriptors features_camera;
    features_camera.image_name = "camera";
    features_camera.keypoints = image_keypoints;
    features_camera.descriptors = image_descriptors;
    theia::HashedImage hashed_camera = feature_matcher_->cascade_hasher_->CreateHashedSiftDescriptors(image_descriptors);

    const theia::Camera& camera = reconstruction_->View(views_to_match[0])->Camera();

    std::unordered_map<theia::Feature, theia::FeatureCorrespondence2D3D> pose_match_map;
    for (const auto& match_view_id : views_to_match) {

        // Get features of matching view
        std::string match_view_name = reconstruction_->View(match_view_id)->Name();
        const theia::KeypointsAndDescriptors& features_match = feature_matcher_->keypoints_and_descriptors_[match_view_name];
        theia::HashedImage& hashed_match = feature_matcher_->hashed_images_[features_match.image_name];

        // Compute matches
        std::vector<theia::IndexedFeatureMatch> putative_matches;
        const double lowes_ratio = feature_matcher_->options_.lowes_ratio;
        feature_matcher_->cascade_hasher_->MatchImages(hashed_camera, features_camera.descriptors,
                                                       hashed_match, features_match.descriptors,
                                                       lowes_ratio, &putative_matches);

        // Get normalized 2D 3D matches
        for (const auto& match_correspondence : putative_matches) {
            const theia::Keypoint& keypoint_camera = features_camera.keypoints[match_correspondence.feature1_ind];
            const theia::Keypoint& keypoint_match = features_match.keypoints[match_correspondence.feature2_ind];

            theia::Feature feature_camera(keypoint_camera.x(), keypoint_camera.y());
            theia::Feature feature_match(keypoint_match.x(), keypoint_match.y());

            const theia::View* match_view = reconstruction_->View(match_view_id);
            const theia::TrackId* track_id_ptr = match_view->GetTrackId(feature_match);

            if (track_id_ptr != nullptr) {
                const theia::Track* track = reconstruction_->Track(*track_id_ptr);

                if (track->IsEstimated()) {
                    theia::FeatureCorrespondence2D3D pose_correspondence;
                    pose_correspondence.feature = camera.PixelToNormalizedCoordinates(feature_camera).hnormalized();
                    pose_correspondence.world_point = track->Point().hnormalized();
                    pose_match_map[feature_camera] = pose_correspondence;
                }
            }
        }
    }
    std::vector<theia::FeatureCorrespondence2D3D> pose_match;
    for (const auto& correspondence : pose_match_map) {
        pose_match.push_back(correspondence.second);
    }

    // Return if number of putative matches is too small
    if (pose_match.size() < options_.reconstruction_estimator_options.min_num_absolute_pose_inliers) {
        return false;
    }

    // Set up the ransac parameters for absolute pose estimation.
    theia::RansacParameters ransac_parameters;
    ransac_parameters.max_iterations = 1000;

    // Compute the reprojection error threshold scaled to account for the image resolution.
    const double resolution_scaled_reprojection_error_threshold_pixels = theia::ComputeResolutionScaledThreshold(
            options_.reconstruction_estimator_options.absolute_pose_reprojection_error_threshold,
            camera.ImageWidth(),
            camera.ImageHeight());

    ransac_parameters.error_thresh = resolution_scaled_reprojection_error_threshold_pixels *
                                     resolution_scaled_reprojection_error_threshold_pixels /
                                     (camera.FocalLength() * camera.FocalLength());

    theia::RansacSummary ransac_summary;
    EstimateCalibratedAbsolutePose(ransac_parameters, theia::RansacType::RANSAC, pose_match, &pose, &ransac_summary);

    return (ransac_summary.inliers.size() >= options_.reconstruction_estimator_options.min_num_absolute_pose_inliers);
}

bool RealtimeReconstructionBuilder::IsInitialized() {
    std::unordered_set<theia::ViewId> estimated_views;
    GetEstimatedViewsFromReconstruction(*reconstruction_, &estimated_views);
    return (estimated_views.size() >= 2);
}

bool RealtimeReconstructionBuilder::ColorizeReconstruction(const std::string& images_path) {
    theia::ColorizeReconstruction(images_path, options_.num_threads, reconstruction_.get());
    return true;
}

bool RealtimeReconstructionBuilder::WritePly(const std::string& output_fullpath) {
    return theia::WritePlyFile(output_fullpath, *reconstruction_, 2);
}

void RealtimeReconstructionBuilder::PrintStatistics(std::ostream& stream,
                                                    bool print_images,
                                                    bool print_reconstruction,
                                                    bool print_view_graph) {
    if (print_images) {
        stream << "Images: ";
        for (const auto& view_id : reconstruction_->ViewIds()) {
            stream << "\n\tid: " << view_id << ", name: " << reconstruction_->View(view_id)->Name();
        }
        stream << "\n";
    }
    if (print_reconstruction) {
        stream << "Reconstruction: ";
        stream << "\n\tNum views = " << reconstruction_->NumViews()
               << "\n\tNum estimated views = " << NumEstimatedViews(*reconstruction_)
               << "\n\tNum tracks = " << reconstruction_->NumTracks()
               << "\n\tNum estimated tracks = " << NumEstimatedTracks(*reconstruction_)
               << "\n";
    }
    if (print_view_graph) {
        stream << "View Graph: ";
        stream << "\n\tNum views = " << view_graph_->NumViews()
               << "\n\tNum edges = " << view_graph_->NumEdges()
               << "\n\tEdges = ";
        for (const auto& edge : view_graph_->GetAllEdges()) {
            theia::ViewIdPair view_pair = edge.first;
            stream << "\t(" << view_pair.first << ", " << view_pair.second << ")";
        }
        stream << "\n";
    }
}

bool RealtimeReconstructionBuilder::AllEstimated() {
    return (reconstruction_->NumViews() == NumEstimatedViews(*reconstruction_));
}

int RealtimeReconstructionBuilder::NumEstimated() {
    return NumEstimatedViews(*reconstruction_);
}

bool RealtimeReconstructionBuilder::IsEstimated(theia::ViewId view_id) {
    return reconstruction_->View(view_id)->IsEstimated();
}

theia::ViewId RealtimeReconstructionBuilder::GetLastAddedViewId() {
    std::vector<theia::ViewId> view_ids = reconstruction_->ViewIds();
    if (!view_ids.empty()) {
        theia::ViewId last_view = *std::max_element(view_ids.begin(), view_ids.end());
        return last_view;
    } else {
        return theia::kInvalidViewId;
    }
}

const RealtimeFeatureMatcher& RealtimeReconstructionBuilder::GetFeatureMatcher() {
    return *feature_matcher_;
}

void RealtimeReconstructionBuilder::SaveImageRetrieval(const std::string& path) {
    image_retrieval_->SaveVisualIndex(path);
}

void RealtimeReconstructionBuilder::SaveViewGraph(const std::string& path) {
    view_graph_->WriteToDisk(path);
}

void RealtimeReconstructionBuilder::SetImageRetrieval(const std::string& path, int num_of_images) {
    image_retrieval_->LoadVisualIndex(path);
    image_retrieval_->SetNumImages(num_of_images);
}

void RealtimeReconstructionBuilder::SetKeypoints(const std::string& images_folder, std::shared_ptr<std::vector<std::string>> image_names, int next_image_idx) {
    for(int i = 0; i < next_image_idx; i++) {
        std::string image_filename;
        theia::GetFilenameFromFilepath(images_folder + image_names->at(i), true, &image_filename);
        theia::FloatImage image(images_folder + image_names->at(i));
        std::vector<theia::Keypoint> image_keypoints;
        std::vector<Eigen::VectorXf> image_descriptors;
        descriptor_extractor_->DetectAndExtractDescriptors(image, &image_keypoints, &image_descriptors);
        feature_matcher_->AddImage(image_filename, image_keypoints, image_descriptors);
    }
}

void RealtimeReconstructionBuilder::SetViewGraph(const std::string& path) {
    view_graph_->ReadFromDisk(path);
}

const theia::Reconstruction& RealtimeReconstructionBuilder::GetReconstruction() {
    return *reconstruction_;
}

const theia::Reconstruction& RealtimeReconstructionBuilder::SetReconstruction(theia::Reconstruction reconstruction) {
    return *reconstruction_ = reconstruction;
}

RealtimeReconstructionBuilder::Options RealtimeReconstructionBuilder::GetOptions() {
    return options_;
}

std::string RealtimeReconstructionBuilder::GetMessage() {
    return reconstruction_message_;
}
