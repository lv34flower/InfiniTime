#include "displayapp/screens/ImageViewer.h"
#include "displayapp/DisplayApp.h"

using namespace Pinetime::Applications::Screens;

ImageViewer* ImageViewer::instance = nullptr;

ImageViewer::ImageViewer(Controllers::FS& fs, Pinetime::Components::LittleVgl& lvgl)
  : fs {fs}, lvgl {lvgl} {
  instance = this;
  LoadDirectory();
  ShowList();
}

ImageViewer::~ImageViewer() {
  instance = nullptr;
  lv_obj_clean(lv_scr_act());
}

void ImageViewer::LoadDirectory() {
  lfs_dir_t dir;
  if (fs.DirOpen("/images", &dir) != 0) {
    return;
  }
  lfs_info info;
  while (fs.DirRead(&dir, &info) > 0) {
    if (info.type != LFS_TYPE_REG) {
      continue;
    }
    std::string name = info.name;
    if (name.size() > 4 && name.substr(name.size() - 4) == ".bin") {
      files.push_back("/images/" + name);
    }
  }
  fs.DirClose(&dir);
}

std::string ImageViewer::DisplayName(const std::string& path) {
  size_t slash = path.rfind('/');
  std::string name = (slash != std::string::npos) ? path.substr(slash + 1) : path;
  if (name.size() > 4 && name.substr(name.size() - 4) == ".bin") {
    name = name.substr(0, name.size() - 4);
  }
  return name;
}

void ImageViewer::ShowList() {
  if (img) {
    lv_obj_del(img);
    img = nullptr;
  }
  state = State::List;

  listWidget = lv_list_create(lv_scr_act(), nullptr);
  lv_obj_set_size(listWidget, LV_HOR_RES, LV_VER_RES);
  lv_obj_align(listWidget, nullptr, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_bg_opa(listWidget, LV_LIST_PART_BG, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_style_local_border_width(listWidget, LV_LIST_PART_BG, LV_STATE_DEFAULT, 0);

  if (files.empty()) {
    lv_list_add_btn(listWidget, nullptr, "No images");
    return;
  }

  for (int i = 0; i < static_cast<int>(files.size()); i++) {
    lv_obj_t* btn = lv_list_add_btn(listWidget, nullptr, DisplayName(files[i]).c_str());
    btn->user_data = reinterpret_cast<void*>(static_cast<intptr_t>(i));
    lv_obj_set_event_cb(btn, ListEventCb);
    lv_obj_set_style_local_pad_top(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 12);
    lv_obj_set_style_local_pad_bottom(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 12);
    lv_obj_set_style_local_margin_bottom(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 5);
    lv_obj_set_style_local_bg_color(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x38, 0x38, 0x38));
    lv_obj_set_style_local_text_color(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_set_style_local_border_color(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x70, 0x70, 0x70));
    lv_obj_set_style_local_border_width(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 1);
    lv_obj_set_style_local_border_opa(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_style_local_radius(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 4);
  }
}

void ImageViewer::ListEventCb(lv_obj_t* obj, lv_event_t event) {
  if (event == LV_EVENT_CLICKED && instance) {
    int idx = static_cast<int>(reinterpret_cast<intptr_t>(obj->user_data));
    instance->ShowImage(static_cast<int16_t>(idx));
  }
}

void ImageViewer::ShowImage(int16_t index) {
  if (listWidget) {
    lv_obj_del(listWidget);
    listWidget = nullptr;
  }
  state = State::Viewing;
  current = index;

  img = lv_img_create(lv_scr_act(), nullptr);
  std::string path = "F:" + files[current];
  lv_img_set_src(img, path.c_str());
  lv_obj_align(img, nullptr, LV_ALIGN_CENTER, 0, 0);

}


void ImageViewer::SwitchImage(int16_t direction) {
  current = static_cast<int16_t>(
    (current + direction + static_cast<int16_t>(files.size())) % static_cast<int16_t>(files.size()));

  std::string path = "F:" + files[current];
  lv_img_set_src(img, path.c_str());
  lv_obj_align(img, nullptr, LV_ALIGN_CENTER, 0, 0);

  lv_obj_invalidate(lv_scr_act());
  lvgl.SetFullRefresh(direction > 0
    ? Pinetime::Components::LittleVgl::FullRefreshDirections::Up
    : Pinetime::Components::LittleVgl::FullRefreshDirections::Down);
}

bool ImageViewer::OnTouchEvent(TouchEvents event) {
  if (state == State::List) {
    if (event == TouchEvents::SwipeDown || event == TouchEvents::SwipeUp) {
      return true;
    }
    return false;
  }
  switch (event) {
    case TouchEvents::SwipeUp:
      SwitchImage(+1);
      return true;
    case TouchEvents::SwipeDown:
      SwitchImage(-1);
      return true;
    case TouchEvents::SwipeRight:
      ShowList();
      return true;
    default:
      return false;
  }
}
