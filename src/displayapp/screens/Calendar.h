#pragma once

#include <lvgl/lvgl.h>
#include <cstdint>
#include <string>
#include "Symbols.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/Controllers.h"
#include "displayapp/DisplayApp.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class Calendar : public Screen {
      public:
        Calendar(Controllers::DateTime& dateTimeController, Controllers::FS& fs);
        ~Calendar() override;

        bool OnTouchEvent(TouchEvents event) override;
        //bool OnTouchEvent(uint16_t x, uint16_t y) override;

      private:
        Controllers::DateTime& dateTimeController;
        Controllers::FS& fs;

        static constexpr uint8_t nColors = 5;
        lv_style_t cellStyles[nColors];
        static constexpr uint8_t nCols = 7;
        static constexpr uint8_t nRows = 7;
        static constexpr const char* dow[7] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};

        // holidays
        const std::string hdir = "/holidays/";
        uint32_t holidays[12];  // array=month bits=day
        void LoadHolidays();
        bool holidaysExists;
        // holidays end

        lv_obj_t* gridDisplay;
        lv_obj_t* titleText;
        lv_obj_t* holidayWarnText;

        int8_t month;
        uint16_t year;

        void Refresh();

        static uint8_t getFirstWeekdayOfMonth(uint16_t year, uint8_t month) {
          if (month < 3) {
            month += 12;
            year--;
          }
          uint8_t h = (year + year / 4 - year / 100 + year / 400 + (13 * month + 8) / 5 + 1) % 7;
          return h;
        }

        static uint8_t  getDaysInMonth(uint16_t year, uint8_t month) {
          static const uint8_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
          if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
            return 29;
          }
          return days_in_month[month - 1];
        }

      };
    }

    template <>
    struct AppTraits<Apps::Calendar> {
      static constexpr Apps app = Apps::Calendar;
      static constexpr const char* icon = Screens::Symbols::calendar;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::Calendar(controllers.dateTimeController, controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}
