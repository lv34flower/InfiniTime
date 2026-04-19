#pragma once

#include <lvgl/lvgl.h>
#include <vector>
#include <string>
#include "displayapp/screens/Screen.h"
#include "displayapp/Controllers.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/LittleVgl.h"
#include "Symbols.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class ImageViewer : public Screen {
      public:
        explicit ImageViewer(Controllers::FS& fs, Pinetime::Components::LittleVgl& lvgl);
        ~ImageViewer() override;
        bool OnTouchEvent(TouchEvents event) override;

      private:
        enum class State { List, Viewing };

        Controllers::FS& fs;
        Pinetime::Components::LittleVgl& lvgl;
        std::vector<std::string> files;
        int16_t current = 0;
        State state = State::List;

        lv_obj_t* listWidget = nullptr;
        lv_obj_t* img = nullptr;

        static ImageViewer* instance;

        void LoadDirectory();
        void ShowList();
        void ShowImage(int16_t index);
        void SwitchImage(int16_t direction);

        static std::string DisplayName(const std::string& path);
        static void ListEventCb(lv_obj_t* obj, lv_event_t event);
      };
    }

    template <>
    struct AppTraits<Apps::ImageViewer> {
      static constexpr Apps app = Apps::ImageViewer;
      static constexpr const char* icon = Screens::Symbols::image;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::ImageViewer(controllers.filesystem, controllers.lvgl);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& fs) {
        lfs_dir_t dir;
        bool exists = (fs.DirOpen("/images", &dir) == 0);
        if (exists) {
          fs.DirClose(&dir);
        }
        return exists;
      }
    };
  }
}
