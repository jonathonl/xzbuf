
#include "xzbuf.hpp"
#include <fstream>

#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include <random>
#include <chrono>
#include <iterator>
#include <sstream>
#include <limits>


class test_base
{
public:
  test_base(const std::string& file_path, std::size_t block_size = std::numeric_limits<std::size_t>::max()):
    file_(file_path),
    block_size_(block_size)
  {
  }
protected:
  static bool file_exists(const std::string& file_path)
  {
    struct stat st;
    return (stat(file_path.c_str(), &st) == 0);
  }

  static bool generate_test_file(const std::string& file_path, std::size_t block_size)
  {
    oxzstream ofs(file_path);
    for (std::size_t i = 0; i < (2048 / 4) && ofs.good(); ++i)
    {
      if (((i * 4) % block_size) == 0)
        ofs.flush();
      ofs << std::setfill('0') << std::setw(3) << i << " " ;
    }
    return ofs.good();
  }
protected:
  std::string file_;
  std::size_t block_size_;
};

class iterator_test: public test_base
{
public:
  using test_base::test_base;
  bool operator()()
  {
    bool ret = false;

    if ((file_exists(file_) &&  std::remove(file_.c_str()) != 0) || !generate_test_file(file_, block_size_))
    {
      std::cerr << "FAILED to generate test file." << std::endl;
    }
    else
    {
      if (!run(file_))
      {
        std::cerr << "FAILED iterator test." << std::endl;
      }
      else
      {
        ret = true;
      }
    }

    return ret;
  }
private:
  bool run(const std::string& file_path)
  {
    ixzbuf sbuf(file_path);
    std::istreambuf_iterator<char> it(&sbuf);
    std::istreambuf_iterator<char> end;

    std::size_t integer = 0;
    for (std::size_t i = 0; it != end; ++i)
    {
      std::stringstream padded_integer;
      for (std::size_t j = 0; j < 4 && it != end; ++j,++it)
        padded_integer.put(*it);
      padded_integer >> integer;

      if (i != integer)
        return false;
    }

    return (integer == 511);
  }
};

class seek_test : public test_base
{
public:
  using test_base::test_base;

  bool operator()()
  {
    bool ret = false;

    if ((file_exists(file_) && std::remove(file_.c_str()) != 0) || !generate_test_file(file_, block_size_))
    {
      std::cerr << "FAILED to generate test file" << std::endl;
    }
    else
    {
      if (!run(file_))
      {
        std::cerr << "FAILED seek test." << std::endl;
      }
      else
      {
        ret = true;
      }
    }

    return ret;
  }
private:
  static bool run(const std::string& file_path)
  {
    ixzstream ifs(file_path);
    std::vector<int> pos_sequence;
    pos_sequence.reserve(128);
    std::mt19937 rg(std::uint32_t(std::chrono::system_clock::now().time_since_epoch().count()));
    for (unsigned i = 0; i < 128 && ifs.good(); ++i)
    {
      int val = 2048 / 4;
      int pos = rg() % val;
      pos_sequence.push_back(pos);
      ifs.seekg(pos * 4, std::ios::beg);
      ifs >> val;
      if (val != pos)
      {
        std::cerr << "Seek failure sequence:" << std::endl;
        for (auto it = pos_sequence.begin(); it != pos_sequence.end(); ++it)
        {
          if (it != pos_sequence.begin())
            std::cerr << ",";
          std::cerr << *it;
        }
        std::cerr << std::endl;
        return false;
      }
    }
    return ifs.good();
  }
};

int main(int argc, char* argv[])
{
  int ret = -1;

  if (argc > 1)
  {
    std::string sub_command = argv[1];
    if (sub_command == "seek")
      ret = !(seek_test("test_seek_file.txt.xz")());
    else if (sub_command == "iterator")
      ret = !(iterator_test("test_iterator_file.txt.xz")() && iterator_test("test_iterator_file_512.txt.xz", 512)() && iterator_test("test_iterator_file_1024.txt.xz", 1024)());
  }

  return ret;
}
