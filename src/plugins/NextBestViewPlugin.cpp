#include "NextBestViewPlugin.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw_gl3.h>

#include "nbv/HelpersOptim.h"

NextBestViewPlugin::NextBestViewPlugin(std::shared_ptr<NextBestView> nbv)
        : next_best_view_(std::move(nbv)) {}

void NextBestViewPlugin::init(igl::opengl::glfw::Viewer *_viewer) {
    ViewerPlugin::init(_viewer);

    // Append mesh for camera
    viewer->append_mesh();
    VIEWER_DATA_NBV = static_cast<unsigned int>(viewer->data_list.size() - 1);

    // Initial camera pose
    Eigen::Affine3d scale(Eigen::Scaling(1.0 / 2.0));
    camera_transformation_ = scale * Eigen::Matrix4d::Identity();

    set_camera();
    show_camera(show_camera_);
}

bool NextBestViewPlugin::post_draw() {
    // Setup window
    float window_width = 350.0f;
    ImGui::SetNextWindowSize(ImVec2(window_width, 0), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(350.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Next best view", nullptr, ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Initialize NBV", ImVec2(-1, 0))) {
        next_best_view_->Initialize();
    }
    if (ImGui::Button("Optimize position", ImVec2(-1, 0))) {
        optimize_position_callback();
    }
    if (ImGui::Button("Optimize rotation", ImVec2(-1, 0))) {
        optimize_rotation_callback();
    }
    if (ImGui::Checkbox("Show next best view camera", &show_camera_)) {
        show_camera(show_camera_);
    }

    // Camera model
    transform_camera();

    ImGui::End();
    return false;
}

void NextBestViewPlugin::optimize_position_callback() {

    // Camera parameters
    unsigned int image_width = next_best_view_->mvs_scene_->images.front().width;
    unsigned int image_height = next_best_view_->mvs_scene_->images.front().height;
    double focal_y = next_best_view_->mvs_scene_->images.front().camera.K(1, 1);

    // Optimization parameters
    arma::vec param_pos = arma::zeros(3, 1);
    param_pos(0) = camera_pos_[0];
    param_pos(1) = camera_pos_[1];
    param_pos(2) = camera_pos_[2];

    optim::algo_settings_t optim_pos_settings;
    optim_pos_settings.err_tol = 0.001;
    optim_pos_settings.iter_max = 1;
    optim_pos_settings.gd_method = 0;
    optim::gd_settings_t gd_settings_pos;
    gd_settings_pos.step_size = 0.1;
    optim_pos_settings.gd_settings = gd_settings_pos;

    OptimPosData optim_pos_data{next_best_view_.get(), camera_rot_, image_width, image_height, focal_y};

    // Run optimization
    auto time_begin = std::chrono::steady_clock::now();

    bool success = optim::gd(param_pos, optim_pos_function, &optim_pos_data, optim_pos_settings);
    camera_pos_[0] = param_pos(0);
    camera_pos_[1] = param_pos(1);
    camera_pos_[2] = param_pos(2);

    auto time_end = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed = time_end - time_begin;
    std::cout << "Elapsed time: " << time_elapsed.count() << " s" << std::endl;

    // Debug
    std::cout << "Parameters: \n" << param_pos;

    // Set camera pose
    set_camera_transformation();
}

void NextBestViewPlugin::optimize_rotation_callback() {

    // Camera parameters
    unsigned int image_width = next_best_view_->mvs_scene_->images.front().width;
    unsigned int image_height = next_best_view_->mvs_scene_->images.front().height;
    double focal_y = next_best_view_->mvs_scene_->images.front().camera.K(1, 1);

    // Optimization parameters
    arma::vec param_rot = arma::zeros(3, 1);
    param_rot(0) = camera_rot_[0];
    param_rot(1) = camera_rot_[1];
    param_rot(2) = camera_rot_[2];

    optim::algo_settings_t optim_rot_settings;
    optim_rot_settings.err_tol = 0.001;
    optim_rot_settings.iter_max = 1;
    optim_rot_settings.gd_method = 0;
    optim::gd_settings_t gd_settings_rot;
    gd_settings_rot.step_size = 0.1;
    optim_rot_settings.gd_settings = gd_settings_rot;

    OptimRotData optim_rot_data{next_best_view_.get(), camera_pos_, image_width, image_height, focal_y};

    // Run optimization
    auto time_begin = std::chrono::steady_clock::now();

    bool success = optim::gd(param_rot, optim_rot_function, &optim_rot_data, optim_rot_settings);
    camera_rot_[0] = param_rot(0);
    camera_rot_[1] = param_rot(1);
    camera_rot_[2] = param_rot(2);

    auto time_end = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_elapsed = time_end - time_begin;
    std::cout << "Elapsed time: " << time_elapsed.count() << " s" << std::endl;

    // Debug
    std::cout << "Parameters: \n" << param_rot;

    // Set camera pose
    set_camera_transformation();
}

void NextBestViewPlugin::set_camera_transformation() {

}

void NextBestViewPlugin::set_camera() {
    // Vertices
    Eigen::MatrixXd tmp_V(5, 3);
    tmp_V << 0, 0, 0,
            -0.75, -0.5, 1,
            -0.75, 0.5, 1,
            0.75, 0.5, 1,
            0.75, -0.5, 1;
    camera_vertices_ = tmp_V;

    // Faces
    Eigen::MatrixXi tmp_F(6, 3);
    tmp_F << 0, 1, 2,
            0, 2, 3,
            0, 3, 4,
            0, 4, 1,
            1, 3, 2,
            1, 4, 3;

    // Set viewer data
    viewer->selected_data_index = VIEWER_DATA_NBV;
    viewer->data().clear();
    viewer->data().set_mesh(tmp_V, tmp_F);
    viewer->data().set_face_based(true);
    viewer->data().set_colors(Eigen::RowVector3d(0, 0, 255) / 255.0);
}

void NextBestViewPlugin::transform_camera() {
    viewer->selected_data_index = VIEWER_DATA_NBV;
    if (camera_vertices_.rows() > 0) {
        Eigen::MatrixXd V = camera_vertices_;
        V = (V.rowwise().homogeneous() * camera_transformation_.transpose()).rowwise().hnormalized();
        viewer->data().set_vertices(V);
    }
}

void NextBestViewPlugin::show_camera(bool visible) {
    viewer->selected_data_index = VIEWER_DATA_NBV;
    viewer->data().show_faces = visible;
    viewer->data().show_lines = visible;
}

// Mouse IO
bool NextBestViewPlugin::mouse_down(int button, int modifier) {
    ImGui_ImplGlfwGL3_MouseButtonCallback(viewer->window, button, GLFW_PRESS, modifier);
    return ImGui::GetIO().WantCaptureMouse;
}

bool NextBestViewPlugin::mouse_up(int button, int modifier) {
    return ImGui::GetIO().WantCaptureMouse;
}

bool NextBestViewPlugin::mouse_move(int mouse_x, int mouse_y) {
    return ImGui::GetIO().WantCaptureMouse;
}

bool NextBestViewPlugin::mouse_scroll(float delta_y) {
    ImGui_ImplGlfwGL3_ScrollCallback(viewer->window, 0.f, delta_y);
    return ImGui::GetIO().WantCaptureMouse;
}

// Keyboard IO
bool NextBestViewPlugin::key_pressed(unsigned int key, int modifiers) {
    ImGui_ImplGlfwGL3_CharCallback(nullptr, key);
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool NextBestViewPlugin::key_down(int key, int modifiers) {
    ImGui_ImplGlfwGL3_KeyCallback(viewer->window, key, 0, GLFW_PRESS, modifiers);
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool NextBestViewPlugin::key_up(int key, int modifiers) {
    ImGui_ImplGlfwGL3_KeyCallback(viewer->window, key, 0, GLFW_RELEASE, modifiers);
    return ImGui::GetIO().WantCaptureKeyboard;
}
