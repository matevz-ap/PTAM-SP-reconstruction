#ifndef REALTIME_RECONSTRUCTION_THEIA_IPCAMERAPLUGIN_H
#define REALTIME_RECONSTRUCTION_THEIA_IPCAMERAPLUGIN_H

#include <glad/glad.h>
#include <imgui/imgui.h>
#include <curl/curl.h>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/ViewerPlugin.h>

#include "reconstruction/RealtimeReconstructionBuilder.h"

class IPCameraPlugin : public igl::opengl::glfw::ViewerPlugin {
public:
    IPCameraPlugin(std::string images_path,
                   std::shared_ptr<theia::RealtimeReconstructionBuilder> reconstruction_builder);
    ~IPCameraPlugin();

    void init(igl::opengl::glfw::Viewer *_viewer) override;
    bool post_draw() override;

    std::shared_ptr<std::vector<std::string>> get_captured_image_names();

    // Mouse IO
    bool mouse_down(int button, int modifier) override;
    bool mouse_up(int button, int modifier) override;
    bool mouse_move(int mouse_x, int mouse_y) override;
    bool mouse_scroll(float delta_y) override;

    // Keyboard IO
    bool key_pressed(unsigned int key, int modifiers) override;
    bool key_down(int key, int modifiers) override;
    bool key_up(int key, int modifiers) override;

private:
    // Viewer data index
    unsigned int VIEWER_DATA_LOCALIZATION;

    // Image input output
    CURL* curl_;
    std::vector<uint8_t> curl_stream_;
    unsigned char* image_data_ = nullptr;

    int saved_frames_count_ = 0;
    std::string images_path_;
    std::shared_ptr<std::vector<std::string>> image_names_;

    // Interface
    GLuint textureID_;
    char url_buffer_[128] = "http://192.168.64.107:8080/photo.jpg";
    // std::string camera_message_;
    bool show_camera_ = false;
    bool auto_localize_ = true;
    bool auto_save_ = false;

    // Reconstruction
    std::shared_ptr<theia::RealtimeReconstructionBuilder> reconstruction_builder_;

    // Localization
    bool prev_localization_success_;
    theia::CalibratedAbsolutePose prev_camera_pose_;
    Eigen::MatrixXd camera_vertices_;
    Eigen::Matrix4d camera_transformation_;

    // Log
    std::ostream& log_stream_ = std::cout;

    // Callback functions
    void capture_image_callback();
    void localize_image_callback();
    void save_image_callback();
    static size_t curl_callback(char *data, size_t size, size_t nmemb, void *userdata);

    // Helpers
    void set_camera();
    void transform_camera();
    void show_camera(bool visible);
};


#endif //REALTIME_RECONSTRUCTION_THEIA_IPCAMERAPLUGIN_H
