#include "displayapp/screens/Calendar.h"
#include "displayapp/DisplayApp.h"

using namespace Pinetime::Applications::Screens;

Calendar::Calendar(Controllers::DateTime& dateTimeController, Controllers::FS& fs)
  : dateTimeController {dateTimeController}, fs {fs} {

  struct colorPair {
    lv_color_t bg;
    lv_color_t fg;
  };

  static constexpr colorPair colors[nColors] = {
    {LV_COLOR_BLACK, LV_COLOR_WHITE}, // Normal day
    {LV_COLOR_GRAY, LV_COLOR_WHITE},  // Today
    {LV_COLOR_BLACK, LV_COLOR_BLUE},  // SAT header
    {LV_COLOR_BLACK, LV_COLOR_RED},   // SUN header | holiday
    {LV_COLOR_GRAY, LV_COLOR_RED},    // Today & holiday
  };

  year = dateTimeController.Year();
  month = static_cast<int>(dateTimeController.Month());

  gridDisplay = lv_table_create(lv_scr_act(), nullptr);
  lv_obj_set_size(gridDisplay, LV_HOR_RES_MAX, LV_VER_RES_MAX - 20);

  for (size_t i = 0; i < nColors; i++) {
    lv_style_init(&cellStyles[i]);

    lv_style_set_border_color(&cellStyles[i], LV_STATE_DEFAULT, lv_color_hex(0xbbada0));
    lv_style_set_border_width(&cellStyles[i], LV_STATE_DEFAULT, 2);
    lv_style_set_bg_opa(&cellStyles[i], LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_bg_color(&cellStyles[i], LV_STATE_DEFAULT, colors[i].bg);
    lv_style_set_pad_top(&cellStyles[i], LV_STATE_DEFAULT, 0);
    lv_style_set_text_color(&cellStyles[i], LV_STATE_DEFAULT, colors[i].fg);

    lv_obj_add_style(gridDisplay, LV_TABLE_PART_CELL1 + i, &cellStyles[i]);
  }

  lv_table_set_col_cnt(gridDisplay, nCols);
  lv_table_set_row_cnt(gridDisplay, nRows);
  for (uint8_t col = 0; col < nCols; col++) {
    static constexpr uint8_t colWidth = LV_HOR_RES_MAX / nCols;
    lv_table_set_col_width(gridDisplay, col, colWidth);
    for (uint8_t row = 0; row < nRows; row++) {
      //grid[row][col].value = 0;
      lv_table_set_cell_type(gridDisplay, row, col, 1);
      lv_table_set_cell_align(gridDisplay, row, col, LV_LABEL_ALIGN_CENTER);

      //lv_table_set_cell_value(gridDisplay, row, col, "T"); // test
    }
    lv_table_set_cell_value(gridDisplay, 0, col, dow[col]);
  }

  // set title row style.
  lv_table_set_cell_type(gridDisplay, 0, 0, 4);
  lv_table_set_cell_type(gridDisplay, 0, 6, 3);

  lv_obj_align(gridDisplay, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

  lv_obj_clean_style_list(gridDisplay, LV_TABLE_PART_BG);

  // year and month text
  titleText = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_width(titleText, LV_HOR_RES);
  lv_label_set_align(titleText, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(titleText, nullptr, LV_ALIGN_IN_TOP_MID, 0, 0);

  // holiday info not exists text
  holidayWarnText = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_width(holidayWarnText, LV_HOR_RES);
  lv_label_set_align(holidayWarnText, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(holidayWarnText, nullptr, LV_ALIGN_IN_TOP_MID, 0, 20);
  lv_obj_set_style_local_text_color(holidayWarnText, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);

  LoadHolidays();
  Refresh(); // date refresh
}

Calendar::~Calendar() {
  lv_obj_clean(lv_scr_act());
}

bool Calendar::OnTouchEvent(TouchEvents event) {
  uint16_t currentYear = year;
  switch (event) {
    case TouchEvents::SwipeLeft:
      ++month;
      break;
    case TouchEvents::SwipeRight:
      --month;
      break;
    case TouchEvents::SwipeUp:
      ++year;
      break;
    case TouchEvents::SwipeDown:
      --year;
      break;
    default:
      return false;
  }
  if (month < 1) {
    month = 12;
    --year;
  }
  if (month > 12) {
    month = 1;
    ++year;
  }

  if (year != currentYear) {
    // reload holidays
    LoadHolidays();
  }

  Refresh();
  return true;
}

void Calendar::Refresh() {
  // title
  lv_label_set_text_fmt(titleText, "%s %u", Controllers::DateTime::MonthShortToStringLow(static_cast<Controllers::DateTime::Months>(month)), year);
  lv_obj_align(titleText, nullptr, LV_ALIGN_IN_TOP_MID, 0, 0);

  if (holidaysExists)
  {
    lv_label_set_text_fmt(holidayWarnText, "");
  }
  else
  {
    lv_label_set_text_fmt(holidayWarnText, "No holiday info");
  }
  lv_obj_align(holidayWarnText, nullptr, LV_ALIGN_IN_TOP_MID, 0, 20);

  // grid
  uint8_t offset = getFirstWeekdayOfMonth(year, month);
  uint8_t count = getDaysInMonth(year, month);
  uint16_t toyear = dateTimeController.Year();
  int8_t tomonth = static_cast<int>(dateTimeController.Month());
  uint8_t today = dateTimeController.Day();

  uint8_t print_day = 1;
  for (uint8_t i = 0; i < nCols * (nRows - 1); ++i) {
    uint8_t col = i % 7;
    uint8_t row = i / 7 + 1;
    if (i >= offset && print_day <= count) {
      // print day
      char buffer[7];
      snprintf(buffer, sizeof(buffer), "%u", print_day);
      lv_table_set_cell_value(gridDisplay, row, col, buffer);

      int th = 1;
      if (toyear == year && tomonth == month && today == print_day) {
        // lv_table_set_cell_type(gridDisplay, row, col, 2);
        th += 1;
      } else {
        // lv_table_set_cell_type(gridDisplay, row, col, 1);
      }

      if (holidaysExists && ((holidays[month - 1] & (0b1 << (print_day - 1))) != 0 ))
      {
        th += 3;
      }
      lv_table_set_cell_type(gridDisplay, row, col, th);

      ++print_day;
    } else {
      lv_table_set_cell_value(gridDisplay, row, col, "");
      lv_table_set_cell_type(gridDisplay, row, col, 1);
    }
  }
}

void Calendar::LoadHolidays() {
  std::string fileName = hdir + std::to_string(year);
  lfs_file_t hFile;
  if (fs.FileOpen(&hFile, fileName.c_str(), LFS_O_RDONLY) != LFS_ERR_OK) {
    holidaysExists = false;
    return;
  }
  holidaysExists = true;

  fs.FileRead(&hFile, reinterpret_cast<uint8_t*>(&holidays[0]), 48);
  fs.FileClose(&hFile);
}
