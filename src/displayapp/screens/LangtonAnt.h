#pragma once

#include <lvgl/lvgl.h>
#include <cstdint>
#include <cstring>
#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "systemtask/WakeLock.h"
#include "Symbols.h"

namespace Pinetime {
  namespace Components {
    class LittleVgl;
  }

  namespace Applications {
    namespace Screens {

      class LangtonAnt : public Screen {
      public:
        LangtonAnt(Pinetime::Components::LittleVgl& lvgl, Pinetime::System::SystemTask& systemTask);
        ~LangtonAnt() override;

        void Refresh() override;
        bool OnButtonPushed() override;
        bool OnTouchEvent(TouchEvents event) override;

      private:
        enum class AppState : uint8_t { SetupAntCount, SetupColorCount, SetupRules, Running };
        enum class Rule : uint8_t { L = 0, R = 1, U = 2, S = 3 };

        struct Ant {
          int16_t x, y;
          uint8_t dir; // 0=North 1=East 2=South 3=West
          bool active = true;
        };

        // wrap=true: 120x120 (exact display fit), wrap=false: 128x128 (4-cell margin each side)
        static constexpr int GRID_W_WORDS = 4; // ceil(128*2/64) = 4 — same for both sizes
        static constexpr int CELL_PX = 2;      // display pixels per cell
        int gridW = 128;
        int gridH = 128;
        int gridOffsetX = 4;
        int gridOffsetY = 4;

        static constexpr uint16_t speedTable[] = {100, 50, 20, 10, 5, 2, 1, 0};
        static constexpr uint8_t speedTableSize = 8;

        static constexpr int8_t DX[] = {0, 1, 0, -1};
        static constexpr int8_t DY[] = {-1, 0, 1, 0};

        // 16KB grid buffer — heap-allocated only during simulation to avoid BSS overflow
        uint64_t (*grid)[GRID_W_WORDS] = nullptr;

        Pinetime::Components::LittleVgl& lvgl;
        Pinetime::System::WakeLock wakeLock;

        AppState state = AppState::SetupAntCount;
        uint8_t antCount = 1;
        uint8_t numColors = 2;
        uint8_t setupAntIndex = 0;
        Rule rules[2][4];
        Ant ants[2] = {};
        bool simulationRunning = false;
        bool wrapAround = false;
        uint8_t speedLevel = 2;

        lv_task_t* taskRefresh = nullptr;
        lv_obj_t* container = nullptr;
        // [ant][color][rule button index 0-3]
        lv_obj_t* ruleButtons[2][4][4] = {};

        void ShowSetupAntCount();
        void ShowSetupColorCount();
        void ShowSetupRules(uint8_t antIdx);
        void StartSimulation();
        void DestroyUI();

        void FreeGrid();
        void DrawCell(int16_t gx, int16_t gy, uint8_t color);
        void StepAnt(uint8_t idx);
        void InitGrid();
        void InitAnts();

        uint8_t GetCell(int16_t x, int16_t y) const;
        void SetCell(int16_t x, int16_t y, uint8_t c);

        void SelectRule(uint8_t antIdx, uint8_t colorIdx, uint8_t ruleIdx);

        static void BtnAntCountCb(lv_obj_t* obj, lv_event_t event);
        static void BtnColorCountCb(lv_obj_t* obj, lv_event_t event);
        static void BtnRuleCb(lv_obj_t* obj, lv_event_t event);
        static void BtnWrapCb(lv_obj_t* obj, lv_event_t event);
        static void BtnNextCb(lv_obj_t* obj, lv_event_t event);
        static void BtnBackCb(lv_obj_t* obj, lv_event_t event);
        static void BtnStartCb(lv_obj_t* obj, lv_event_t event);
      };
    }

    template <>
    struct AppTraits<Apps::LangtonAnt> {
      static constexpr Apps app = Apps::LangtonAnt;
      static constexpr const char* icon = Screens::Symbols::eye;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::LangtonAnt(controllers.lvgl, *controllers.systemTask);
      }

      static bool IsAvailable(Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
