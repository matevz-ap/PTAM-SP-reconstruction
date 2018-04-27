#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui_impl_glfw_gl3.h>
#include <igl/readOFF.h>
#include <igl/opengl/glfw/Viewer.h>

#include "helpers.h"
#include "MenuPlugin.h"
#include "CameraPlugin.h"
#include "RealtimeReconstructionBuilder.h"
#include "ReconstructionPlugin.h"

int main(int argc, char *argv[]) {

    // Initialization
    std::string camera_device = "/dev/video0";
    // std::string images_path = "../webcam_images/";
    // std::string reconstruction_path = "../webcam_reconstruction/";

    std::string images_path = "../castle_images/";
    std::string reconstruction_path = "../castle_reconstruction/";

    std::string calibration_file = reconstruction_path + "prior_calibration.txt";

    // Read the mesh
    Eigen::MatrixXd vertices;
    Eigen::MatrixXi faces;
    igl::readOFF("../assets/bunny.off", vertices, faces);

    // Initialize the viewer
    igl::opengl::glfw::Viewer viewer;
    viewer.core.is_animating = true;
    viewer.core.set_rotation_type(igl::opengl::ViewerCore::RotationType::ROTATION_TYPE_TRACKBALL);
    viewer.data().point_size = 3;

    // Setup viewer callbacks for ImGui
    viewer.callback_init = [] (igl::opengl::glfw::Viewer& v) -> bool {
        // Setup ImGui
        ImGui::CreateContext();
        ImGui_ImplGlfwGL3_Init(v.window, false);
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameRounding = 5.0f;
        return false;
    };

    viewer.callback_pre_draw = [] (igl::opengl::glfw::Viewer& v) -> bool {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();
        return false;
    };

    viewer.callback_post_draw = [] (igl::opengl::glfw::Viewer& v) -> bool {
        ImGui::Render();
        return false;
    };

    viewer.callback_shutdown = [] (igl::opengl::glfw::Viewer& v) -> bool {
        // ImGui cleanup
        ImGui_ImplGlfwGL3_Shutdown();
        ImGui::DestroyContext();
        return false;
    };

    // Attach a menu plugin
    MenuPlugin menu_plugin;
    viewer.plugins.push_back(&menu_plugin);

    // Attach reconstruction plugin
    theia::RealtimeReconstructionBuilderOptions options;

    theia::CameraIntrinsicsPrior intrinsics_prior = read_calibration(calibration_file);
    options.intrinsics_prior = intrinsics_prior;
    options.reconstruction_estimator_options.reconstruction_estimator_type =
            theia::ReconstructionEstimatorType::INCREMENTAL;

    ReconstructionPlugin reconstruction_plugin(options, images_path, reconstruction_path);
    viewer.plugins.push_back(&reconstruction_plugin);

    // Attach camera plugin
    // int image_width = intrinsics_prior.image_width;
    // int image_height = intrinsics_prior.image_height;
    // CameraPlugin camera_plugin(camera_device, image_width, image_height, images_path);
    // viewer.plugins.push_back(&camera_plugin);

    // Start viewer
    // viewer.data().set_mesh(vertices, faces);
    viewer.data().set_vertices(vertices);
    viewer.data().add_points(vertices, Eigen::RowVector3d(1,0,0));
    viewer.launch();
}