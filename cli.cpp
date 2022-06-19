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

        theia::CameraIntrinsicsPrior intrinsics_prior = ReadCalibration(calibration_path);
        RealtimeReconstructionBuilder::Options options = SetRealtimeReconstructionBuilderOptions();
        options.intrinsics_prior = intrinsics_prior;
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
    else {
            std::string root_folder = "../dataset_new/";

            // Dataset setting
            std::string project_folder = root_folder + "opeka/";

            int num_images = 30;
            std::string image_ext = ".jpg";
            std::string images_folder = project_folder + "images/";
            std::string reconstruction_folder = project_folder + "reconstruction/";
            std::string calibration_file = project_folder + "prior_calibration.txt";

            // Setup reconstruction objects
            theia::CameraIntrinsicsPrior intrinsics_prior = ReadCalibration(calibration_file);
            RealtimeReconstructionBuilder::Options options = SetRealtimeReconstructionBuilderOptions();
            options.intrinsics_prior = intrinsics_prior;
            auto reconstruction_builder = std::make_shared<RealtimeReconstructionBuilder>(options);
            auto mvs_scene = std::make_shared<MVS::Scene>(options.num_threads);
            auto quality_measure = std::make_shared<QualityMeasure>(mvs_scene);

            // Attach reconstruction plugin
            ReconstructionPlugin::Parameters reconstruction_parameters;
            std::shared_ptr<std::vector<std::string>> image_names = std::make_shared<std::vector<std::string>>();
            for (int i = 0; i <= num_images; i++) {
                std::stringstream ss;
                ss << std::setw(3) << std::setfill('0') << std::to_string(i);
                image_names->emplace_back("frame" + ss.str() + image_ext);
            }

            ReconstructionPlugin reconstruction_plugin(reconstruction_parameters,
                                                    images_folder,
                                                    reconstruction_folder,
                                                    image_names,
                                                    reconstruction_builder,
                                                    mvs_scene,
                                                    quality_measure,
                                                    false);
        if (argv[1] == std::string("init")) {
            reconstruction_plugin.initialize_callback();
            // reconstruction_plugin.extend_all_callback();
            // reconstruction_plugin.save_reconstruction_state("recon_state");
        }
        else if (argv[1] == std::string("extend")) {
            reconstruction_plugin.load_scene_callback();
            reconstruction_plugin.extend_callback();
            reconstruction_plugin.save_scene_callback();
        }
    }
    return 0;
}