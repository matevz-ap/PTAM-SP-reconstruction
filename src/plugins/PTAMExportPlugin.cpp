//
// Created by luka on 21. 05. 20.
//
#include "PTAMExportPlugin.h"

#include <imgui/imgui.h>
#include <cereal/include/cereal/archives/binary.hpp>
#include "util/PTAMExporter.h"

void PTAMExportPlugin::init(igl::opengl::glfw::Viewer *_viewer) {
    ViewerPlugin::init(_viewer);

    viewer->append_mesh();
    VIEWER_DATA_MESH = static_cast<unsigned int>(viewer->data_list.size() - 1);

    viewer->append_mesh();
    VIEWER_DATA_CAL_POINTS= static_cast<unsigned int>(viewer->data_list.size() - 1);
}

bool PTAMExportPlugin::pre_draw() {
    ImGuizmo::BeginFrame();
    return false;
}

bool PTAMExportPlugin::post_draw() {
    // Setup window
    float window_width = 500.0f;
    ImGui::SetNextWindowSize(ImVec2(window_width, 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(350.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Izvoz modela", nullptr, ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Ponastavi kamero", ImVec2(-1, 0))) {
        center_object_callback();
    }

    if (ImGui::Button("Postavi model", ImVec2(-1, 0))) {
        set_mesh();
    }

    if (ImGui::Button("Postavi tocke", ImVec2(-1, 0))) {
        pointsSet = false;
        set_cal_points();
        pointsSet = true;
    } else if (pointsSet){
        set_cal_points();
    }

    if (ImGui::Checkbox("Viden model", &parameters_.show_mesh)) {
        show_mesh(parameters_.show_mesh);
    }

    if (ImGui::TreeNodeEx("Kalibracijske tocke", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(ImColor(1.0f, 0.0f, 0.0f, 1.0f), "Tocka 1");
        ImGui::SameLine();
        if (ImGui::Button("Center 1")) {
            center_cal_point(0);
        }
        ImGui::SameLine();
        if (ImGui::Button("Random 1")) {
            random_cal_point(0);
        }
        ImGui::TextUnformatted("Model (x,y,z):");
        ImGui::SameLine();
        ImGui::PushItemWidth(110);
        ImGui::InputFloat("##calPoint1ModelX", &parameters_.calPoint1Model[0], 0.01, 0.3, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint1ModelY", &parameters_.calPoint1Model[1], 0.01, 0.3, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint1ModelZ", &parameters_.calPoint1Model[2], 0.01, 0.3, 4);
        ImGui::PopItemWidth();
        ImGui::TextUnformatted("Ref (x,y,z):");
        ImGui::SameLine();
        ImGui::PushItemWidth(110);
        ImGui::InputFloat("##calPoint1RefX", &parameters_.calPoint1Ref[0], 0.01, 0.3, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint1RefY", &parameters_.calPoint1Ref[1], 0.01, 0.3, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint1RefZ", &parameters_.calPoint1Ref[2], 0.01, 0.3, 4);
        ImGui::PopItemWidth();

        ImGui::TextColored(ImColor(0.0f, 1.0f, 0.0f, 1.0f), "Tocka 2");
        ImGui::SameLine();
        if (ImGui::Button("Center 2")) {
            center_cal_point(1);
        }
        ImGui::SameLine();
        if (ImGui::Button("Random 2")) {
            random_cal_point(1);
        }
        ImGui::TextUnformatted("Model (x,y,z):");
        ImGui::SameLine();
        ImGui::PushItemWidth(110);
        ImGui::InputFloat("##calPoint2ModelX", &parameters_.calPoint2Model[0], 0.01, 0.3, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint2ModelY", &parameters_.calPoint2Model[1], 0.01, 0.3, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint2ModelZ", &parameters_.calPoint2Model[2], 0.01, 0.3, 4);
        ImGui::PopItemWidth();
        ImGui::TextUnformatted("Ref (x,y,z):");
        ImGui::SameLine();
        ImGui::PushItemWidth(110);
        ImGui::InputFloat("##calPoint2RefX", &parameters_.calPoint2Ref[0], 0.01, 0.3, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint2RefY", &parameters_.calPoint2Ref[1], 0.01, 0.3, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint2RefZ", &parameters_.calPoint2Ref[2], 0.01, 0.3, 4);
        ImGui::PopItemWidth();

        ImGui::TextColored(ImColor(0.0f, 0.0f, 1.0f, 1.0f), "Tocka 3");
        ImGui::SameLine();
        if (ImGui::Button("Center 3")) {
            center_cal_point(2);
        }
        ImGui::SameLine();
        if (ImGui::Button("Random 3")) {
            random_cal_point(2);
        }
        ImGui::TextUnformatted("Model (x,y,z):");
        ImGui::SameLine();
        ImGui::PushItemWidth(110);
        ImGui::InputFloat("##calPoint3ModelX", &parameters_.calPoint3Model[0], 0.01, 0.1, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint3ModelY", &parameters_.calPoint3Model[1], 0.01, 0.1, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint3ModelZ", &parameters_.calPoint3Model[2], 0.01, 0.1, 4);
        ImGui::PopItemWidth();
        ImGui::TextUnformatted("Ref (x,y,z):");
        ImGui::SameLine();
        ImGui::PushItemWidth(110);
        ImGui::InputFloat("##calPoint3RefX", &parameters_.calPoint3Ref[0], 0.01, 0.1, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint3RefY", &parameters_.calPoint3Ref[1], 0.01, 0.1, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint3RefZ", &parameters_.calPoint3Ref[2], 0.01, 0.1, 4);
        ImGui::PopItemWidth();

        ImGui::TextColored(ImColor(1.0f, 1.0f, 0.0f, 1.0f), "Tocka 4");
        ImGui::SameLine();
        if (ImGui::Button("Center 4")) {
            center_cal_point(3);
        }
        ImGui::SameLine();
        if (ImGui::Button("Random 4")) {
            random_cal_point(3);
        }
        ImGui::TextUnformatted("Model (x,y,z):");
        ImGui::SameLine();
        ImGui::PushItemWidth(110);
        ImGui::InputFloat("##calPoint4ModelX", &parameters_.calPoint4Model[0], 0.01, 0.1, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint4ModelY", &parameters_.calPoint4Model[1], 0.01, 0.1, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint4ModelZ", &parameters_.calPoint4Model[2], 0.01, 0.1, 4);
        ImGui::PopItemWidth();
        ImGui::TextUnformatted("Ref (x,y,z):");
        ImGui::SameLine();
        ImGui::PushItemWidth(110);
        ImGui::InputFloat("##calPoint4RefX", &parameters_.calPoint4Ref[0], 0.01, 0.1, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint4RefY", &parameters_.calPoint4Ref[1], 0.01, 0.1, 4);
        ImGui::SameLine();
        ImGui::InputFloat("##calPoint4RefZ", &parameters_.calPoint4Ref[2], 0.01, 0.1, 4);
        ImGui::PopItemWidth();

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Skala modela", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* items[] = { "Tocka 1", "Tocka 2", "Tocka 3", "Tocka 4" };
        static const char* current_item_1 = NULL;

        if (ImGui::BeginCombo("##scale_point_1", current_item_1))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                bool is_selected = (current_item_1 == items[n]);
                if (ImGui::Selectable(items[n], is_selected)) {
                    parameters_.scalePoint1 = n;
                    current_item_1 = items[n];
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        static const char* current_item_2 = NULL;
        if (ImGui::BeginCombo("##scale_point_2", current_item_2))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                bool is_selected = (current_item_2 == items[n]);
                if (ImGui::Selectable(items[n], is_selected)) {
                    parameters_.scalePoint2 = n;
                    current_item_2 = items[n];
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::TextUnformatted("Razdalja med tockama (cm):");
        ImGui::SameLine();
        ImGui::PushItemWidth(115);
        ImGui::InputFloat("##scale_distance", &parameters_.scaleDistance, 0.01, 0.1, 4);


        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("3D model scene")) {
        ImGui::TextUnformatted("Ime datoteke:");
        ImGui::SameLine();
        ImGui::PushItemWidth(200);
        ImGui::InputText("##model_export_path", parameters_.modelPath, 128);
        if (ImGui::Button("Izvozi 3D model", ImVec2(-1,0))) {
            export_model();
        }
        ImGui::TreePop();
    }

    if (ImGui::Button("Izvozi rekonstrukcijo", ImVec2(-1,0))) {
        export_scene();
    }

    ImGui::End();
    return false;
}

void PTAMExportPlugin::center_object_callback() {
    viewer->selected_data_index = VIEWER_DATA_MESH;
    if (viewer->data().V.rows() > 0) {
        Eigen::Vector3d min_point = viewer->data().V.colwise().minCoeff();
        Eigen::Vector3d max_point = viewer->data().V.colwise().maxCoeff();
        Eigen::Vector3d center = viewer->data().V.colwise().mean();
        viewer->core.camera_base_translation = (-center).cast<float>();
        viewer->core.camera_translation.setConstant(0);

        Eigen::Vector3d diff = (max_point-min_point).array().abs();
        viewer->core.camera_base_zoom = static_cast<float>(2.0 / diff.maxCoeff());
        viewer->core.camera_zoom = 1.0;
    }
}

void PTAMExportPlugin::center_cal_point(int i) {
    Eigen::Vector3d center = cal_points_.row(i);
    viewer->core.camera_base_translation = (-center).cast<float>();
    viewer->core.camera_translation.setConstant(0);
}

void PTAMExportPlugin::random_cal_point(int i) {
    int j = rand() % mvs_scene_->mesh.vertices.size();
    MVS::Mesh::Vertex vertex = mvs_scene_->mesh.vertices[j];
    switch (i) {
        case 0:
            parameters_.calPoint1Model[0] = vertex[0];
            parameters_.calPoint1Model[1] = vertex[1];
            parameters_.calPoint1Model[2] = vertex[2];
            break;
        case 1:
            parameters_.calPoint2Model[0] = vertex[0];
            parameters_.calPoint2Model[1] = vertex[1];
            parameters_.calPoint2Model[2] = vertex[2];
            break;
        case 2:
            parameters_.calPoint3Model[0] = vertex[0];
            parameters_.calPoint3Model[1] = vertex[1];
            parameters_.calPoint3Model[2] = vertex[2];
            break;
        case 3:
            parameters_.calPoint4Model[0] = vertex[0];
            parameters_.calPoint4Model[1] = vertex[1];
            parameters_.calPoint4Model[2] = vertex[2];
            break;
    }
}

void PTAMExportPlugin::set_cal_points() {
    viewer->selected_data_index = VIEWER_DATA_CAL_POINTS;
    viewer->data().clear();

    if (pointsSet) {
        for (int i = 0; i < 3; i++) {
            cal_points_(0, i) = parameters_.calPoint1Model[i];
            cal_points_(1, i) = parameters_.calPoint2Model[i];
            cal_points_(2, i) = parameters_.calPoint3Model[i];
            cal_points_(3, i) = parameters_.calPoint4Model[i];
        }
    } else {
        srand(time(NULL));
        int num_vertices = mvs_scene_->mesh.vertices.size();
        std::set<int> selectedPoints;
        while (selectedPoints.size() < 4) {
            selectedPoints.insert(rand() % num_vertices);
        }

        int c = 0;
        for (auto i : selectedPoints) {
            MVS::Mesh::Vertex vertex = mvs_scene_->mesh.vertices[i];
            cal_points_(c, 0) = vertex[0];
            cal_points_(c, 1) = vertex[1];
            cal_points_(c, 2) = vertex[2];
            switch (c) {
                case 0:
                    parameters_.calPoint1Model[0] = vertex[0];
                    parameters_.calPoint1Model[1] = vertex[1];
                    parameters_.calPoint1Model[2] = vertex[2];
                    break;
                case 1:
                    parameters_.calPoint2Model[0] = vertex[0];
                    parameters_.calPoint2Model[1] = vertex[1];
                    parameters_.calPoint2Model[2] = vertex[2];
                    break;
                case 2:
                    parameters_.calPoint3Model[0] = vertex[0];
                    parameters_.calPoint3Model[1] = vertex[1];
                    parameters_.calPoint3Model[2] = vertex[2];
                    break;
                case 3:
                    parameters_.calPoint4Model[0] = vertex[0];
                    parameters_.calPoint4Model[1] = vertex[1];
                    parameters_.calPoint4Model[2] = vertex[2];
                    break;
            }
            c++;
        }
    }


    Eigen::MatrixXd colors(4, 3);
    colors(0, 0) = 1;
    colors(0, 1) = 0;
    colors(0, 2) = 0;

    colors(1, 0) = 0;
    colors(1, 1) = 1;
    colors(1, 2) = 0;

    colors(2, 0) = 0;
    colors(2, 1) = 0;
    colors(2, 2) = 1;

    colors(3, 0) = 1;
    colors(3, 1) = 1;
    colors(3, 2) = 0;



    viewer->data().set_points(cal_points_, colors);
    viewer->data().show_lines = true;
    viewer->data().show_faces = true;
}

void PTAMExportPlugin::show_mesh(bool visible) {
    viewer->selected_data_index = VIEWER_DATA_MESH;
    viewer->data().show_faces = visible;
    viewer->data().show_lines = visible;
}

void PTAMExportPlugin::set_mesh() {
    // Select viewer data
    viewer->selected_data_index = VIEWER_DATA_MESH;
    viewer->data().clear();

    // Add vertices
    int num_vertices = mvs_scene_->mesh.vertices.size();
    Eigen::MatrixXd V(num_vertices, 3);
    for (int i = 0; i < num_vertices; i++) {
        MVS::Mesh::Vertex vertex = mvs_scene_->mesh.vertices[i];
        V(i, 0) = vertex[0];
        V(i, 1) = vertex[1];
        V(i, 2) = vertex[2];
    }
    // Add faces
    int num_faces = mvs_scene_->mesh.faces.size();
    Eigen::MatrixXi F(num_faces, 3);
    for (int i = 0; i < num_faces; i++) {
        MVS::Mesh::Face face = mvs_scene_->mesh.faces[i];
        F(i, 0) = face[0];
        F(i, 1) = face[1];
        F(i, 2) = face[2];
    }
    viewer->data().set_mesh(V, F);
    viewer->data().set_colors( Eigen::RowVector3d(1, 1, 1));
    show_mesh(parameters_.show_mesh);

    // Add texture if available
    if (!mvs_scene_->mesh.faceTexcoords.IsEmpty()) {

        // Set UVs
        int num_texcoords = mvs_scene_->mesh.faceTexcoords.size();
        Eigen::MatrixXd TC(num_texcoords, 2);
        for (int i = 0; i < num_texcoords; i++) {
            MVS::Mesh::TexCoord texcoord = mvs_scene_->mesh.faceTexcoords[i];
            TC(i, 0) = texcoord[0];
            TC(i, 1) = texcoord[1];
        }
        Eigen::MatrixXi FTC(num_faces, 3);
        for (int i = 0; i < num_faces; i++) {
            FTC(i, 0) = i*3 + 0;
            FTC(i, 1) = i*3 + 1;
            FTC(i, 2) = i*3 + 2;
        }
        viewer->data().set_uv(TC, FTC);

        // Set texture
        SEACAVE::Image8U3 img = mvs_scene_->mesh.textureDiffuse;
        int width = img.width();
        int height = img.height();

        Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> R(width, height);
        Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> G(width, height);
        Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> B(width, height);

        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                Pixel8U pixel = img.getPixel(j, i);
                R(i, j) = pixel.r;
                G(i, j) = pixel.g;
                B(i, j) = pixel.b;
            }
        }

        viewer->data().set_colors(Eigen::RowVector3d(1, 1, 1));
        viewer->data().set_texture(R.rowwise().reverse(), G.rowwise().reverse(), B.rowwise().reverse());
        viewer->data().show_texture = true;
    }
}

void PTAMExportPlugin::export_scene() {
    log_stream_ << "EXPORT started" << std::endl;
    theia::Reconstruction reconstruction = reconstruction_builder_->GetReconstruction();

    PTAMExporter ptamExport;
    std::vector<theia::ViewId> viewIds = reconstruction.ViewIds();
    std::sort(viewIds.begin(), viewIds.end());

    ptamExport.scalePoint1 = parameters_.scalePoint1;
    ptamExport.scalePoint2 = parameters_.scalePoint2;
    ptamExport.scaleDistance = parameters_.scaleDistance;

    // Add frames
    for (const auto& vid : viewIds) {
        if (reconstruction_builder_->IsEstimated(vid)) {
            const theia::View *v = reconstruction.View(vid);
            log_stream_ << "VIEW: " << vid << " - " << v->Name()  << std::endl;
            ptamExport.addFrame(vid, v->Name());
        }
    }
    // Add points
    std::vector<theia::TrackId> trackPoints = reconstruction.TrackIds();
    for (int i = 0; i < mvs_scene_->mesh.vertices.size(); i++) {
        auto mp = mvs_scene_->mesh.vertices[i];
        std::vector<std::pair<theia::TrackId, double>> distances;
        for (auto& trackID : trackPoints) {
            if (auto tp = reconstruction.Track(trackID)) {
                Eigen::Vector3d posA(tp->Point().x(), tp->Point().y(), tp->Point().z());
                Eigen::Vector3d posB(mp.x, mp.y, mp.z);
                double d = (posA-posB).norm();
                distances.emplace_back(std::pair<theia::TrackId, double>(trackID, d));
            }
        }
        std::sort(distances.begin(), distances.end(), [](const std::pair<theia::TrackId, double> &x, const std::pair<theia::TrackId, double> &y) {
            return x.second < y.second;
        });

        PTAMPoint ptamP(mp.x, mp.y, mp.z);
        for (const auto& vid : viewIds) {
            if (reconstruction_builder_->IsEstimated(vid)) {
                for (int x = 0; x < distances.size() && x < 3; x++) {
                    if (auto track = reconstruction.Track(distances[x].first)) {
                        if (track->ViewIds().find(vid) != track->ViewIds().end()) {
                            const theia::View *v = reconstruction.View(vid);
                            const theia::Camera &cam = v->Camera();
                            const Eigen::Vector4d a(mp.x, mp.y, mp.z, 1);
                            Eigen::Vector2d b(0,0);
                            double d = cam.ProjectPoint(a, &b);
                            if (d > 0)
                                ptamP.addPoint(x, vid, b.x(), b.y());
                            break;
                        }
                    }
                }
            }
        }

        if (ptamP.numberOfImagePoints() > 0)
            ptamExport.addPoint(ptamP);
    }
    std::cout << mvs_scene_->mesh.vertices.size() << std::endl;
    std::cout << ptamExport.points.size() << std::endl;
    // Add calibration
    PTAMPoint cal1M(parameters_.calPoint1Model[0], parameters_.calPoint1Model[1], parameters_.calPoint1Model[2]);
    PTAMPoint cal1W(parameters_.calPoint1Ref[0], parameters_.calPoint1Ref[1], parameters_.calPoint1Ref[2]);
    ptamExport.addCalPoint(cal1M, cal1W);
    PTAMPoint cal2M(parameters_.calPoint2Model[0], parameters_.calPoint2Model[1], parameters_.calPoint2Model[2]);
    PTAMPoint cal2W(parameters_.calPoint2Ref[0], parameters_.calPoint2Ref[1], parameters_.calPoint2Ref[2]);
    ptamExport.addCalPoint(cal2M, cal2W);
    PTAMPoint cal3M(parameters_.calPoint3Model[0], parameters_.calPoint3Model[1], parameters_.calPoint3Model[2]);
    PTAMPoint cal3W(parameters_.calPoint3Ref[0], parameters_.calPoint3Ref[1], parameters_.calPoint3Ref[2]);
    ptamExport.addCalPoint(cal3M, cal3W);
    PTAMPoint cal4M(parameters_.calPoint4Model[0], parameters_.calPoint4Model[1], parameters_.calPoint4Model[2]);
    PTAMPoint cal4W(parameters_.calPoint4Ref[0], parameters_.calPoint4Ref[1], parameters_.calPoint4Ref[2]);
    ptamExport.addCalPoint(cal4M, cal4W);

    std::ofstream ptamFile(reconstructionFolder+"installer",  std::ios::binary);
    {
        cereal::BinaryOutputArchive oarchive(ptamFile);
        oarchive(ptamExport);
    }
    ptamFile.close();
    log_stream_ << "EXPORT finished" << std::endl;
}

void PTAMExportPlugin::export_model() {
    log_stream_ << "EXPORT 3D model started" << std::endl;
    Eigen::MatrixXf model(3, 4);
    // model.resize(3, 4);
    Eigen::MatrixXf ref(3, 4);
    // ref.resize(3, 4);
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 3; j++) {
    //         model(j, i) = parameters_.calPoint1Model[j];
    //         ref(j, i) = parameters_.calPoint1Ref[j];
    //     }
    //     model(3, i) = 1;
    //     ref(3, i) = 1;
    // }

    model.col(0) = Eigen::Vector3f(parameters_.calPoint1Model[0], parameters_.calPoint1Model[1], parameters_.calPoint1Model[2]);
    ref.col(0) = Eigen::Vector3f(parameters_.calPoint1Ref[0], parameters_.calPoint1Ref[1], parameters_.calPoint1Ref[2]);

    model.col(1) = Eigen::Vector3f(parameters_.calPoint2Model[0], parameters_.calPoint2Model[1], parameters_.calPoint2Model[2]);
    ref.col(1) = Eigen::Vector3f(parameters_.calPoint2Ref[0], parameters_.calPoint2Ref[1], parameters_.calPoint2Ref[2]);

    model.col(2) = Eigen::Vector3f(parameters_.calPoint3Model[0], parameters_.calPoint3Model[1], parameters_.calPoint3Model[2]);
    ref.col(2) = Eigen::Vector3f(parameters_.calPoint3Ref[0], parameters_.calPoint3Ref[1], parameters_.calPoint3Ref[2]);

    model.col(3) = Eigen::Vector3f(parameters_.calPoint4Model[0], parameters_.calPoint4Model[1], parameters_.calPoint4Model[2]);
    ref.col(3) = Eigen::Vector3f(parameters_.calPoint4Ref[0], parameters_.calPoint4Ref[1], parameters_.calPoint4Ref[2]);

    // model.col(0) = Eigen::Vector4f(parameters_.calPoint1Model[0], parameters_.calPoint1Model[1], parameters_.calPoint1Model[2], 1);
    // ref.col(0) = Eigen::Vector4f(parameters_.calPoint1Ref[0], parameters_.calPoint1Ref[1], parameters_.calPoint1Ref[2], 1);

    // model.col(1) = Eigen::Vector4f(parameters_.calPoint2Model[0], parameters_.calPoint2Model[1], parameters_.calPoint2Model[2], 1);
    // ref.col(1) = Eigen::Vector4f(parameters_.calPoint2Ref[0], parameters_.calPoint2Ref[1], parameters_.calPoint2Ref[2], 1);

    // model.col(2) = Eigen::Vector4f(parameters_.calPoint3Model[0], parameters_.calPoint3Model[1], parameters_.calPoint3Model[2], 1);
    // ref.col(2) = Eigen::Vector4f(parameters_.calPoint3Ref[0], parameters_.calPoint3Ref[1], parameters_.calPoint3Ref[2], 1);

    // model.col(3) = Eigen::Vector4f(parameters_.calPoint4Model[0], parameters_.calPoint4Model[1], parameters_.calPoint4Model[2], 1);
    // ref.col(3) = Eigen::Vector4f(parameters_.calPoint4Ref[0], parameters_.calPoint4Ref[1], parameters_.calPoint4Ref[2], 1);

    auto transform = Eigen::umeyama(model, ref);

    for (int i = 0; i < mvs_scene_->mesh.vertices.size(); i++) {
        MVS::Mesh::Vertex vertex = mvs_scene_->mesh.vertices[i];
        auto v = Eigen::Vector4f(vertex[0], vertex[1], vertex[2], 1);
        log_stream_ << transform << transform.cols() << transform.rows() << std::endl;
        log_stream_ << v << v.cols() << v.rows() << std::endl;

        v = transform * v;
        mvs_scene_->mesh.vertices[i][0] = v.x();
        mvs_scene_->mesh.vertices[i][1] = v.y();
        mvs_scene_->mesh.vertices[i][2] = v.z();
    }

    mvs_scene_->Save(reconstructionFolder + parameters_.modelPath);

    // for (int i = 0; i < mvs_scene_->mesh.vertices.size(); i++) {
    //     MVS::Mesh::Vertex vertex = mvs_scene_->mesh.vertices[i];
    //     auto v = Eigen::Vector4f(vertex[0], vertex[1], vertex[2], 1);
    //     v = transform.inverse() * v;
    //     mvs_scene_->mesh.vertices[i][0] = v.x();
    //     mvs_scene_->mesh.vertices[i][1] = v.y();
    //     mvs_scene_->mesh.vertices[i][2] = v.z();
    // }

    log_stream_ << "EXPORT 3D model finished" << std::endl;
}
