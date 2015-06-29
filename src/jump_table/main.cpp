#include <iostream>
#include <cstdint>

#include <elfio/elfio.hpp>
#include "tinyformat.h"

int main(int argc, char* argv[])
{
  ELFIO::elfio elf_reader;
  elf_reader.load(argv[1]);

  try {
    tfm::printf("ELF file class: ");
    switch (elf_reader.get_class()) {
      case ELFCLASS32: tfm::printfln("ELF32"); break;
      case ELFCLASS64: tfm::printfln("ELF64"); break;
      default: throw 1; break;
    }

    tfm::printf("ELF file encoding: ");
    switch (elf_reader.get_encoding()) {
      case ELFDATA2LSB: tfm::printfln("Little endian"); break;
      case ELFDATA2MSB: tfm::printfln("Big endian"); break;
      default: throw 2; break;
    }

    auto sec_number = elf_reader.sections.size();
    tfm::printfln("Number of sections: %d", sec_number);
//    for (uint32_t i = 0; i < sec_number; ++i) {
//      const auto psection = elf_reader.sections[i];
//      tfm::printfln("%2d %-25s %-7d 0x8%x",
//                    i, psection->get_name(), psection->get_size(), psection->get_address());
//    }

    auto seg_number = elf_reader.segments.size();
    tfm::printfln("Number of segments: %d", seg_number);
//    for (uint32_t i = 0; i < seg_number; ++i) {
//      const auto psegment = elf_reader.segments[i];
//      tfm::printfln("%2d 0x8%x 0x%-8x 0x%-4x %3d",
//                    i, psegment->get_flags(), psegment->get_virtual_address(),
//                    psegment->get_file_size(), psegment->get_memory_size());
//    }

    auto rodata_base_address = 0;
    auto rodata  = (const char*){nullptr};
    auto rodata_size = uint32_t{0};

    auto data_base_address = 0;
    auto data = (const char*){nullptr};
    auto data_size = uint32_t{0};

    for (uint32_t i = 0; i < sec_number; ++i) {
      auto psection = elf_reader.sections[i];

      if (psection->get_name() == ".rodata") {
        rodata_base_address = psection->get_address();
        rodata = psection->get_data();
        rodata_size = psection->get_size();
      }

      if (psection->get_name() == ".data") {
        data_base_address = psection->get_address();
        data = psection->get_data();
        data_size = psection->get_size();
      }

      if ((data != nullptr) && (rodata != nullptr)) break;
    }


    //=== dumping jump table
    if (rodata == nullptr) throw 4;
    tfm::printfln("Section .rodata of size %d bytes found at range [0x%x, 0x%x)",
                  rodata_size, rodata_base_address, rodata_base_address + rodata_size);

    auto start_jtable_address = std::stoul(argv[2], nullptr, 0x10);
    auto stop_jtable_address = std::stoul(argv[3], nullptr, 0x10);

    if ((start_jtable_address < rodata_base_address) ||
        (stop_jtable_address > rodata_base_address + rodata_size)) throw 5;

    auto start_jtable_entry = (const uint8_t*)(rodata + (start_jtable_address - rodata_base_address));
    auto stop_jtable_entry = (const uint8_t*)(rodata + (stop_jtable_address - rodata_base_address));

    tfm::printfln("Dumping %d entries in .rodata section (0x%x) of [0x%x, 0x%x)",
                  (stop_jtable_address - start_jtable_address) / sizeof(uint8_t),
                  rodata_base_address, start_jtable_address, stop_jtable_address);
    auto count = uint32_t{0};
    for (auto jmp_entry = start_jtable_entry; jmp_entry < stop_jtable_entry; ++jmp_entry) {
      auto jmp_entry_value = *jmp_entry;
      tfm::printf("0x%x; ", jmp_entry_value);
      ++count; if (count % 16 == 0) tfm::printfln("");
    }

    //=== dumping virtual PC table
    if (data == nullptr) throw 6;
    tfm::printfln("\n\nSection .data of size %d bytes found at range [0x%x, 0x%x)",
                  data_size, data_base_address, data_base_address + data_size);

    auto start_pctable_address = std::stoul(argv[4], nullptr, 0x10);
    auto stop_pctable_address = std::stoul(argv[5], nullptr, 0x10);

    if ((start_pctable_address < data_base_address) ||
        (stop_pctable_address > data_base_address + data_size)) throw 7;

    auto start_pctable_entry = (const uint8_t*)(data + (start_pctable_address - data_base_address));
    auto stop_pctable_entry = (const uint8_t*)(data + (stop_pctable_address - data_base_address));

    tfm::printfln("Dumping %d entries in .data section (0x%x) of [0x%x, 0x%x)",
                  (stop_pctable_address - start_pctable_address) / sizeof(uint8_t),
                  data_base_address, start_pctable_address, stop_pctable_address);
    count = 0;
    for (auto pc_entry = start_pctable_entry; pc_entry < stop_pctable_entry; ++pc_entry) {
      auto pc_entry_value = *pc_entry;
      tfm:printf("0x%x; ", pc_entry_value);
      ++count; if (count % 16 == 0) tfm::printfln("");
    }
  }
  catch (int e) {
    switch (e) {
      case 1: tfm::printfln("Invalid file class"); break;
      case 2: tfm::printfln("Invalid file encoding"); break;
      case 3: tfm::printfln("[start, stop) range must be located in rodata"); break;
      case 4: tfm::printfln("Cannot read .rodata section"); break;
      case 5: tfm::printfln("Start address must be greater than base address of rodata section"); break;
      case 6: tfm::printfln("Cannot read .data section"); break;
      case 7: tfm::printfln("Start address must be greater than base address of data section"); break;
      default: break;
    }
  }
  catch (const std::exception& expt) {
    tfm::printfln("%s", expt.what());
  }

  return 0;
}

