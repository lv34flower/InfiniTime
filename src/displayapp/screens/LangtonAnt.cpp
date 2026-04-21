#include "displayapp/screens/LangtonAnt.h"
#include "displayapp/LittleVgl.h"
#include "displayapp/InfiniTimeTheme.h"

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <new>

using namespace Pinetime::Applications::Screens;

// ─── Constructor / Destructor ────────────────────────────────────────────────

LangtonAnt::LangtonAnt(Pinetime::Components::LittleVgl& lvgl, Pinetime::System::SystemTask& systemTask)
  : lvgl {lvgl}, wakeLock {systemTask} {
  // Default rules: classic RL pattern
  for (int a = 0; a < 2; a++) {
    rules[a][0] = Rule::R;
    rules[a][1] = Rule::L;
    rules[a][2] = Rule::R;
    rules[a][3] = Rule::L;
  }
  ShowSetupAntCount();
}

LangtonAnt::~LangtonAnt() {
  if (taskRefresh != nullptr) {
    lv_task_del(taskRefresh);
  }
  FreeGrid();
  lv_obj_clean(lv_scr_act());
}

// ─── Grid ────────────────────────────────────────────────────────────────────

uint8_t LangtonAnt::GetCell(int16_t x, int16_t y) const {
  int bit = x * 2;
  return (grid[y][bit / 64] >> (bit % 64)) & 0x3;
}

void LangtonAnt::SetCell(int16_t x, int16_t y, uint8_t c) {
  int bit = x * 2;
  int w = bit / 64;
  int b = bit % 64;
  grid[y][w] = (grid[y][w] & ~(3ULL << b)) | (static_cast<uint64_t>(c) << b);
}

void LangtonAnt::FreeGrid() {
  delete[] grid;
  grid = nullptr;
}

void LangtonAnt::InitGrid() {
  std::memset(grid, 0, sizeof(uint64_t) * gridH * GRID_W_WORDS);
}

void LangtonAnt::InitAnts() {
  if (antCount == 1) {
    ants[0] = {static_cast<int16_t>(gridW / 2), static_cast<int16_t>(gridH / 2), 0, true};
  } else {
    ants[0] = {static_cast<int16_t>(gridW / 2 - 5), static_cast<int16_t>(gridH / 2), 0, true};
    ants[1] = {static_cast<int16_t>(gridW / 2 + 5), static_cast<int16_t>(gridH / 2), 0, true};
  }
}

// ─── Drawing ─────────────────────────────────────────────────────────────────

void LangtonAnt::DrawCell(int16_t gx, int16_t gy, uint8_t color) {
  int16_t cx = gx - gridOffsetX;
  int16_t cy = gy - gridOffsetY;
  if (cx < 0 || cx >= 120 || cy < 0 || cy >= 120) {
    return;
  }
  static const lv_color_t colorTable[4] = {
    LV_COLOR_WHITE, LV_COLOR_BLACK, LV_COLOR_RED, Colors::green
  };
  lv_color_t pixBuf[CELL_PX * CELL_PX];
  std::fill(pixBuf, pixBuf + CELL_PX * CELL_PX, colorTable[color]);
  int16_t dx = cx * CELL_PX;
  int16_t dy = cy * CELL_PX;
  lv_area_t area = {dx, dy, static_cast<int16_t>(dx + CELL_PX - 1), static_cast<int16_t>(dy + CELL_PX - 1)};
  lvgl.FlushDisplay(&area, pixBuf);
}

// ─── Simulation ──────────────────────────────────────────────────────────────

void LangtonAnt::StepAnt(uint8_t idx) {
  Ant& a = ants[idx];
  if (!a.active) return;
  if (a.x < 0 || a.x >= gridW || a.y < 0 || a.y >= gridH) {
    a.active = false;
    return;
  }
  uint8_t c = GetCell(a.x, a.y);
  switch (rules[idx][c]) {
    case Rule::L: a.dir = (a.dir + 3) % 4; break;
    case Rule::R: a.dir = (a.dir + 1) % 4; break;
    case Rule::U: a.dir = (a.dir + 2) % 4; break;
    case Rule::S: break;
  }
  uint8_t nc = (c + 1) % numColors;
  SetCell(a.x, a.y, nc);
  DrawCell(a.x, a.y, nc);
  a.x += DX[a.dir];
  a.y += DY[a.dir];
  if (wrapAround) {
    if (a.x < 0) a.x += gridW;
    else if (a.x >= gridW) a.x -= gridW;
    if (a.y < 0) a.y += gridH;
    else if (a.y >= gridH) a.y -= gridH;
  }
}

void LangtonAnt::Refresh() {
  if (!simulationRunning) return;
  for (uint8_t i = 0; i < antCount; i++) {
    StepAnt(i);
  }
  // Stop simulation only when all ants are inactive
  bool anyActive = false;
  for (uint8_t i = 0; i < antCount; i++) {
    if (ants[i].active) { anyActive = true; break; }
  }
  if (!anyActive) simulationRunning = false;
}

// ─── Input ───────────────────────────────────────────────────────────────────

bool LangtonAnt::OnButtonPushed() {
  if (state == AppState::Running) {
    lv_task_del(taskRefresh);
    taskRefresh = nullptr;
    simulationRunning = false;
    wakeLock.Release();
    FreeGrid();
    ShowSetupAntCount();
    return true;
  }
  return false;
}

bool LangtonAnt::OnTouchEvent(TouchEvents event) {
  if (state == AppState::Running) {
    if (event == TouchEvents::SwipeLeft && speedLevel < speedTableSize - 1) {
      speedLevel++;
      lv_task_set_period(taskRefresh, speedTable[speedLevel]);
    } else if (event == TouchEvents::SwipeRight && speedLevel > 0) {
      speedLevel--;
      lv_task_set_period(taskRefresh, speedTable[speedLevel]);
    }
    return true;
  }
  return false;
}

// ─── UI ──────────────────────────────────────────────────────────────────────

void LangtonAnt::DestroyUI() {
  lv_obj_clean(lv_scr_act());
  container = nullptr;
  std::memset(ruleButtons, 0, sizeof(ruleButtons));
}

static LangtonAnt* SelfFromScreen() {
  return static_cast<LangtonAnt*>(lv_obj_get_user_data(lv_scr_act()));
}

static lv_obj_t* MakeBtn(lv_obj_t* parent, const char* label, lv_event_cb_t cb,
                          void* userdata, int16_t x, int16_t y, int16_t w, int16_t h) {
  lv_obj_t* btn = lv_btn_create(parent, nullptr);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_size(btn, w, h);
  lv_obj_set_event_cb(btn, cb);
  lv_obj_set_user_data(btn, userdata);
  lv_obj_t* lbl = lv_label_create(btn, nullptr);
  lv_label_set_text(lbl, label);
  lv_obj_align(lbl, btn, LV_ALIGN_CENTER, 0, 0);
  return btn;
}

void LangtonAnt::ShowSetupAntCount() {
  DestroyUI();
  state = AppState::SetupAntCount;

  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_user_data(scr, this);

  lv_obj_t* title = lv_label_create(scr, nullptr);
  lv_label_set_text_static(title, "# of ants");
  lv_obj_align(title, scr, LV_ALIGN_IN_TOP_MID, 0, 20);

  MakeBtn(scr, "1", BtnAntCountCb, reinterpret_cast<void*>(uintptr_t {1}), 30, 90, 80, 60);
  MakeBtn(scr, "2", BtnAntCountCb, reinterpret_cast<void*>(uintptr_t {2}), 130, 90, 80, 60);

  lv_obj_t* wrapBtn = lv_btn_create(scr, nullptr);
  lv_btn_set_checkable(wrapBtn, true);
  lv_obj_set_pos(wrapBtn, 30, 170);
  lv_obj_set_size(wrapBtn, 180, 40);
  lv_obj_set_event_cb(wrapBtn, BtnWrapCb);
  if (wrapAround) {
    lv_obj_add_state(wrapBtn, LV_STATE_CHECKED);
  }
  lv_obj_t* wrapLbl = lv_label_create(wrapBtn, nullptr);
  lv_label_set_text_static(wrapLbl, wrapAround ? "Wrap: ON" : "Wrap: OFF");
  lv_obj_align(wrapLbl, wrapBtn, LV_ALIGN_CENTER, 0, 0);
}

void LangtonAnt::ShowSetupColorCount() {
  DestroyUI();
  state = AppState::SetupColorCount;

  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_user_data(scr, this);

  lv_obj_t* title = lv_label_create(scr, nullptr);
  lv_label_set_text_static(title, "# of colors");
  lv_obj_align(title, scr, LV_ALIGN_IN_TOP_MID, 0, 20);

  const char* labels[3] = {"2", "3", "4"};
  for (uint8_t n = 2; n <= 4; n++) {
    MakeBtn(scr, labels[n - 2], BtnColorCountCb,
            reinterpret_cast<void*>(static_cast<uintptr_t>(n)),
            20 + (n - 2) * 72, 90, 60, 60);
  }
}

void LangtonAnt::ShowSetupRules(uint8_t antIdx) {
  DestroyUI();
  state = AppState::SetupRules;
  setupAntIndex = antIdx;

  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_user_data(scr, this);

  char titleBuf[20];
  if (antCount == 2) {
    snprintf(titleBuf, sizeof(titleBuf), "Ant %d / 2", antIdx + 1);
  } else {
    snprintf(titleBuf, sizeof(titleBuf), "Ant 1");
  }
  lv_obj_t* title = lv_label_create(scr, nullptr);
  lv_label_set_text(title, titleBuf);
  lv_obj_align(title, scr, LV_ALIGN_IN_TOP_MID, 0, 3);

  static const lv_color_t colorFill[4] = {
    LV_COLOR_WHITE, LV_COLOR_BLACK, LV_COLOR_RED, Colors::green
  };
  static const char* const ruleLabels[4] = {"L", "R", "U", "S"};

  uint8_t displayColors = numColors;
  int16_t btnW = (240 - 30) / 4;

  for (uint8_t ci = 0; ci < displayColors; ci++) {
    int16_t rowY = 28 + ci * 40;

    lv_obj_t* ind = lv_obj_create(scr, nullptr);
    lv_obj_set_size(ind, 26, 32);
    lv_obj_set_pos(ind, 2, rowY + 4);
    lv_obj_set_style_local_bg_color(ind, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, colorFill[ci]);
    lv_obj_set_style_local_border_color(ind, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
    lv_obj_set_style_local_border_width(ind, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 1);
    lv_obj_set_style_local_radius(ind, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);

    for (uint8_t ri = 0; ri < 4; ri++) {
      uintptr_t key = (static_cast<uintptr_t>(antIdx) << 16)
                    | (static_cast<uintptr_t>(ci) << 8)
                    | ri;
      lv_obj_t* btn = MakeBtn(scr, ruleLabels[ri], BtnRuleCb,
                               reinterpret_cast<void*>(key),
                               30 + ri * btnW, rowY, btnW - 2, 36);
      ruleButtons[antIdx][ci][ri] = btn;

      lv_color_t bg = (static_cast<uint8_t>(rules[antIdx][ci]) == ri) ? Colors::highlight : Colors::bgAlt;
      lv_obj_set_style_local_bg_color(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, bg);
    }
  }

  int16_t navY = 28 + displayColors * 40 + 4;
  if (navY > 200) navY = 200;

  if (antCount == 2 && antIdx == 0) {
    MakeBtn(scr, "Next ->", BtnNextCb, nullptr, 130, navY, 105, 32);
  } else if (antCount == 2 && antIdx == 1) {
    MakeBtn(scr, "<- Back", BtnBackCb, nullptr, 0, navY, 105, 32);
    MakeBtn(scr, "Start", BtnStartCb, nullptr, 130, navY, 105, 32);
  } else {
    MakeBtn(scr, "Start", BtnStartCb, nullptr, 65, navY, 110, 32);
  }
}

void LangtonAnt::SelectRule(uint8_t antIdx, uint8_t colorIdx, uint8_t ruleIdx) {
  rules[antIdx][colorIdx] = static_cast<Rule>(ruleIdx);
  for (uint8_t ri = 0; ri < 4; ri++) {
    lv_obj_t* btn = ruleButtons[antIdx][colorIdx][ri];
    if (btn == nullptr) continue;
    lv_color_t bg = (ri == ruleIdx) ? Colors::highlight : Colors::bgAlt;
    lv_obj_set_style_local_bg_color(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, bg);
  }
}

void LangtonAnt::StartSimulation() {
  if (wrapAround) {
    gridW = 120; gridH = 120; gridOffsetX = 0; gridOffsetY = 0;
  } else {
    gridW = 128; gridH = 128; gridOffsetX = 4; gridOffsetY = 4;
  }
  grid = new (std::nothrow) uint64_t[gridH][GRID_W_WORDS];
  if (grid == nullptr) {
    // Not enough heap — stay on setup screen
    return;
  }
  DestroyUI();
  state = AppState::Running;
  InitGrid();
  InitAnts();

  // White LVGL background — LVGL renders this once; ant pixels overlay on top via FlushDisplay
  lv_obj_t* bgObj = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(bgObj, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_pos(bgObj, 0, 0);
  lv_obj_set_style_local_bg_color(bgObj, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_set_style_local_border_width(bgObj, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_radius(bgObj, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_all(bgObj, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);

  taskRefresh = lv_task_create(RefreshTaskCallback, speedTable[speedLevel], LV_TASK_PRIO_MID, this);
  simulationRunning = true;
  wakeLock.Lock();
}

// ─── Callbacks ───────────────────────────────────────────────────────────────

void LangtonAnt::BtnAntCountCb(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) return;
  auto* self = SelfFromScreen();
  if (self == nullptr) return;
  auto count = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(obj)));
  self->antCount = count;
  self->ShowSetupColorCount();
}

void LangtonAnt::BtnColorCountCb(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) return;
  auto* self = SelfFromScreen();
  if (self == nullptr) return;
  auto n = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(obj)));
  self->numColors = n;
  self->ShowSetupRules(0);
}

void LangtonAnt::BtnRuleCb(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) return;
  auto* self = SelfFromScreen();
  if (self == nullptr) return;
  uintptr_t key = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(obj));
  uint8_t antIdx   = (key >> 16) & 0xFF;
  uint8_t colorIdx = (key >> 8)  & 0xFF;
  uint8_t ruleIdx  =  key        & 0xFF;
  self->SelectRule(antIdx, colorIdx, ruleIdx);
}

void LangtonAnt::BtnWrapCb(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_VALUE_CHANGED) return;
  auto* self = SelfFromScreen();
  if (self == nullptr) return;
  self->wrapAround = (lv_obj_get_state(obj, LV_BTN_PART_MAIN) & LV_STATE_CHECKED) != 0;
  lv_obj_t* lbl = lv_obj_get_child(obj, nullptr);
  if (lbl != nullptr) {
    lv_label_set_text_static(lbl, self->wrapAround ? "Wrap: ON" : "Wrap: OFF");
  }
}

void LangtonAnt::BtnNextCb(lv_obj_t* /*obj*/, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) return;
  auto* self = SelfFromScreen();
  if (self != nullptr) self->ShowSetupRules(1);
}

void LangtonAnt::BtnBackCb(lv_obj_t* /*obj*/, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) return;
  auto* self = SelfFromScreen();
  if (self != nullptr) self->ShowSetupRules(0);
}

void LangtonAnt::BtnStartCb(lv_obj_t* /*obj*/, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) return;
  auto* self = SelfFromScreen();
  if (self != nullptr) self->StartSimulation();
}
