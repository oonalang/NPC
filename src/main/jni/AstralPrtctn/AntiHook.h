void* GetAnogs() {
    static void* cached = nullptr;
    if (cached) return cached;
    FILE* fp = fopen(OBFUSCATE("/proc/self/maps"), "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, OBFUSCATE("libanogs.so")) && strstr(line, OBFUSCATE("r-xp"))) {
                uintptr_t start = 0;
                sscanf(line, OBFUSCATE("%lx"), &start);
                uintptr_t end = 0;
                char perms[5] = {0};
                sscanf(line, OBFUSCATE("%lx-%lx %4s"), &start, &end, perms);
                if (start && end > start) {
                    size_t size = end - start;
                    void* address = (void*)start;
                    void* temp = mmap(nullptr, size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                    if (temp != MAP_FAILED) {
                        memcpy(temp, address, size);
                        void* remapped = mremap(temp, size, size, MREMAP_MAYMOVE | MREMAP_FIXED, address);
                        if (remapped != MAP_FAILED) {
                            int prot = PROT_READ | PROT_EXEC;
                            mprotect(address, size, prot);
                        } else {
                            munmap(temp, size);
                        }
                    }
                    fclose(fp);
                    cached = (void*)start;
                    return cached;
                }
            }
        }
        fclose(fp);
    }
    return nullptr;
}

uintptr_t EncOff(uintptr_t offs) {
    static uintptr_t key = 0;
    if (!key) {
        key = (uintptr_t)GetAnogs;
        key = (key << 16) | (key >> 48);
        if (key == 0) key = 1;
    }
    return offs ^ key;
}

uintptr_t DecOff(uintptr_t encrypted) {
    static uintptr_t key = 0;
    if (!key) {
        key = (uintptr_t)GetAnogs;
        key = (key << 16) | (key >> 48);
        if (key == 0) key = 1;
    }
    return encrypted ^ key;
}

void Kooh(uintptr_t encoffs, void* hook, void** original) {
    void* Anogs = GetAnogs();
    if (Anogs) {
        uintptr_t real = DecOff(encoffs);
        void* target = (void*)((uintptr_t)Anogs + real);
        Tools::Hook(target, hook, original);
    }
}

void ApplyHook(uintptr_t& offs, void* hook, void** orig) {
    Kooh(offs, hook, orig);
    usleep(3000000);
    static uintptr_t key = 0;
    if (!key) {
        key = (uintptr_t)ApplyHook;
        key = (key << 16) | (key >> 48);
        if (key == 0) key = 1;
    }
    offs = offs ^ key;
}
