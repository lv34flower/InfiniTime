#include <fstream>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <tuple>
#include <set>
#include <sstream>
#include <cstdint>
#include <iostream>

const int cnt = 4;
const int years[] = {2023, 2024, 2025, 2026};

int main()
{
  std::map<int, std::map<int, std::set<int> > > holidays;
  std::ifstream file("holidays.txt");
  std::string line;

  while (std::getline(file, line))
  {
    int year, month, day;
    std::replace(line.begin(), line.end(), '-', ' ');
    std::istringstream iss(line);
    iss >> year >> month >> day;
    holidays[year][month].insert(day);
  }

  for (int year = years[0]; year < cnt + years[0]; ++year)
  {
    uint32_t calendar[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (auto it_m = holidays[year].begin(); it_m != holidays[year].end(); ++it_m)  // month
    {
      for (auto it_d = it_m->second.begin(); it_d != it_m->second.end(); ++it_d)  // day
      {
        calendar[it_m->first - 1] |= 0b1 << (*it_d - 1);
        std::cout << year << "-" << it_m->first << "-" << *it_d << std::endl;
      }
    }

    std::string outname = std::to_string(year);// + std::string(".bin");
    std::ofstream outputFile(outname.c_str(), std::ios::binary);
    if (!outputFile) {
      std::cerr << "File open error" << std::endl;
      return 1;
    }
    outputFile.write(reinterpret_cast<const char*>(calendar), 12 * sizeof(uint32_t));
  }
}
