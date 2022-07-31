//
// Created by luka on 21. 05. 20.
//

#ifndef REALTIME_RECONSTRUCTION_PTAMEXPORTPLUGIN_H
#define REALTIME_RECONSTRUCTION_PTAMEXPORTPLUGIN_H

#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/ViewerPlugin.h>
#include <imguizmo/ImGuizmo.h>
#include <OpenMVS/MVS.h>
#include <reconstruction/RealtimeReconstructionBuilder.h>

class PTAMExportPlugin : public igl::opengl::glfw::ViewerPlugin {
public:

    PTAMExportPlugin(const std::string &reconstruction_path, std::shared_ptr<RealtimeReconstructionBuilder> reconstruction_builder, std::shared_ptr<MVS::Scene> mvs_scene) :
    reconstruction_builder_(reconstruction_builder),
    mvs_scene_(mvs_scene),
    reconstructionFolder(reconstruction_path){}

    struct Parameters {
        bool show_mesh = true;

        float calPoint1Model[3] = { 0.0f, 0.0f, 0.0f };
        float calPoint1Ref[3] = { 0.12f, 0.12f, 0.0f };

        float calPoint2Model[3] = { 0.0f, 0.0f, 0.0f };
        float calPoint2Ref[3] = { 0.18f, 0.18f, 0.0f };

        float calPoint3Model[3] = { 0.0f, 0.0f, 0.0f };
        float calPoint3Ref[3] = { 0.24f, 0.18f, 0.0f };

        float calPoint4Model[3] = { 0.0f, 0.0f, 0.0f };
        float calPoint4Ref[3] = { 0.30f, 0.12f, 0.0f };

        int scalePoint1 = -1;
        int scalePoint2 = -1;
        float scaleDistance = 0;

        char modelPath[128] = "3d_model_in_ref.mvs";
    };

    void init(igl::opengl::glfw::Viewer *_viewer) override;
    bool pre_draw() override;
    bool post_draw() override;
    void export_scene();
    void export_model();

private:
    // Parameters
    Parameters parameters_;
    std::string reconstructionFolder;

    // Viewer data indices
    unsigned int VIEWER_DATA_MESH;
    unsigned int VIEWER_DATA_CAL_POINTS;

    bool pointsSet = false;
    std::ostream& log_stream_ = std::cout;

    // Reconstruction
    std::shared_ptr<RealtimeReconstructionBuilder> reconstruction_builder_;
    std::shared_ptr<MVS::Scene> mvs_scene_;
    // Calibration points
    Eigen::MatrixXd cal_points_ = Eigen::MatrixXd(4, 3);

    void center_object_callback();
    void center_cal_point(int i);
    void random_cal_point(int i);
    void set_mesh();
    void show_mesh(bool visible);
    void set_cal_points();
};

#endif //REALTIME_RECONSTRUCTION_PTAMEXPORTPLUGIN_H
