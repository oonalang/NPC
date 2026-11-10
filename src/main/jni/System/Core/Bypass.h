#pragma once

bool (*orig_bypass)(void *ins);
bool hook_bypass(void *ins) {
    return false;
}

#if defined(__aarch64__)

inline const char *armFalse = "C0 03 5F D6";

#endif

struct range {
    uintptr_t Irt, Ind;
    struct Iter {
      uintptr_t val;
      bool operator!=(const Iter& it) const {
        return val <= it.val; 
      }
      uintptr_t operator*() const {
        return val; 
      }
      void operator++() {
        val += 4; 
      }
    };
    Iter begin() const { 
      return {Irt}; 
    }
    Iter end() const {
      return {Ind}; 
    }
};

inline void InitializeProtection() {
    Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x204218), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x258B6C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x259670), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x3055A0), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x3075C4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x307764), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x30E234), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x40F360), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4102B4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x44BC90), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x497E64), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x3893D8), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x40ECB4), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x40FECC), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x411C8C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x42125C), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x412208), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x427364), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x431158), (void*)hook_bypass, (void**)&orig_bypass);
Tools::Hook((void*)getAbsoluteAddress(OBFUSCATE("libanogs.so"), 0x4477E0), (void*)hook_bypass, (void**)&orig_bypass);
    for (auto offs : range{0x1, 0x1000}) {
        MemoryPatch::createWithHex("libanogs.so", offs, armFalse).Modify();
    }
    
}
    
    
