#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <igl/readOFF.h>
#include <igl/opengl/glfw/Viewer.h>

#include "reconstruction/Helpers.h"
#include "reconstruction/RealtimeReconstructionBuilder.h"
#include "nbv/QualityMeasure.h"
#include "plugins/ReconstructionPlugin.h"
#include "plugins/EditMeshPlugin.h"
#include "plugins/NextBestViewPlugin.h"

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    if (argv[1] == "download_ply") {

    }
    else if (argv[1] == std::string("from_folder")) {
        std::string images_folder = argv[2];
        std::string calibration_path = argv[3];
        std::string output_folder = argv[4];

        std::shared_ptr<std::vector<std::string>> image_names = std::make_shared<std::vector<std::string>>();
        for (const auto & entry : fs::directory_iterator(images_folder)) {
            image_names->emplace_back(entry.path().filename());
        }
        std::sort(image_names->begin(), image_names->end());

        RealtimeReconstructionBuilder::Options options = SetRealtimeReconstructionBuilderOptions();
        options.intrinsics_prior = ReadCalibration(calibration_path);
        auto reconstruction_builder = std::make_shared<RealtimeReconstructionBuilder>(options);
        auto mvs_scene = std::make_shared<MVS::Scene>(options.num_threads);
        auto quality_measure = std::make_shared<QualityMeasure>(mvs_scene);
        ReconstructionPlugin::Parameters reconstruction_parameters;
        ReconstructionPlugin reconstruction_plugin(reconstruction_parameters,
                                                    images_folder,
                                                    output_folder,
                                                    image_names,
                                                    reconstruction_builder,
                                                    mvs_scene,
                                                    quality_measure,
                                                    false);

        reconstruction_plugin.initialize_callback();
    }
    else if (argv[1] == std::string("init")) {
        std::string images_folder = argv[2];
        std::string calibration_path = argv[3]; // needs change to params instead of file
        std::string output_folder = argv[4];

        std::shared_ptr<std::vector<std::string>> image_names = std::make_shared<std::vector<std::string>>();
        for (const auto & entry : fs::directory_iterator(images_folder)) {
            image_names->emplace_back(entry.path().filename());
        }
        std::sort(image_names->begin(), image_names->end());

        RealtimeReconstructionBuilder::Options options = SetRealtimeReconstructionBuilderOptions();
        options.intrinsics_prior = ReadCalibration(calibration_path);
        auto reconstruction_builder = std::make_shared<RealtimeReconstructionBuilder>(options);
        auto mvs_scene = std::make_shared<MVS::Scene>(options.num_threads);
        auto quality_measure = std::make_shared<QualityMeasure>(mvs_scene);
        ReconstructionPlugin::Parameters reconstruction_parameters;
        ReconstructionPlugin reconstruction_plugin(reconstruction_parameters,
                                                    images_folder,
                                                    output_folder,
                                                    image_names,
                                                    reconstruction_builder,
                                                    mvs_scene,
                                                    quality_measure,
                                                    false);

        reconstruction_plugin.initialize_callback();
        reconstruction_plugin.save_reconstruction_state(output_folder);
    }
    else if (argv[1] == std::string("extend")) {
        std::string images_folder = argv[2];
        std::string calibration_path = argv[3]; // needs change to params instead of file
        std::string output_folder = argv[4];
        int next_image_idx = std::stoi(argv[5]);

        std::shared_ptr<std::vector<std::string>> image_names = std::make_shared<std::vector<std::string>>();
        for (const auto & entry : fs::directory_iterator(images_folder)) {
            image_names->emplace_back(entry.path().filename());
        }
        std::sort(image_names->begin(), image_names->end());
            
        RealtimeReconstructionBuilder::Options options = SetRealtimeReconstructionBuilderOptions();
        options.intrinsics_prior = ReadCalibration(calibration_path);
        auto reconstruction_builder = std::make_shared<RealtimeReconstructionBuilder>(options);
        auto mvs_scene = std::make_shared<MVS::Scene>(options.num_threads);
        auto quality_measure = std::make_shared<QualityMeasure>(mvs_scene);
        ReconstructionPlugin::Parameters reconstruction_parameters;

        std::ifstream os(output_folder + "/reconstruction", std::ios::binary);
        cereal::PortableBinaryInputArchive iarchive(os);
        theia::Reconstruction recon;
        iarchive(recon);
        reconstruction_builder->SetReconstruction(recon);
        reconstruction_builder->SetImageRetrieval(output_folder + "/image_retrieval", 2);
        reconstruction_builder->SetViewGraph(output_folder + "/view_graph");
        reconstruction_builder->SetKeypoints(images_folder, image_names, next_image_idx);
        reconstruction_parameters.next_image_idx = next_image_idx;
        ReconstructionPlugin reconstruction_plugin(reconstruction_parameters,
                                                    images_folder,
                                                    output_folder,
                                                    image_names,
                                                    reconstruction_builder,
                                                    mvs_scene,
                                                    quality_measure,
                                                    false);
        reconstruction_plugin.extend_callback();
    }
    return 0;
}