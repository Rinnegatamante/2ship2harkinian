#!/usr/bin/env python
"""
vita-make-fself-debug.py

Creates a Razor-compatible SELF from a VitaSDK VELF file.
Embeds the raw VELF uncompressed (preserving section headers, symbols, debug info).
Fixes p_paddr in program headers for correct Razor address resolution.

Produces two files:
  output.self        - compressed (guaranteed to load, no section data for Razor)
  output_debug.self  - uncompressed with full section headers (for Razor)

Usage:
    python vita-make-fself-debug.py input.velf output.self
"""

import struct
import sys
import os
import hashlib
import zlib

# ============================================================
# Constants
# ============================================================
HEADER_LEN = 0x1000

PT_LOAD = 1
PT_SCE_RELA = 0x60000000

ET_SCE_EXEC = 0xFE00
ET_SCE_RELEXEC = 0xFE04

CTRL4_SIZE = 0x50
CTRL5_SIZE = 0x110
CTRL6_SIZE = 0x110
CTRL7_SIZE = 0x50
CTRL_TOTAL_SIZE = CTRL4_SIZE + CTRL5_SIZE + CTRL6_SIZE + CTRL7_SIZE  # 0x2C0

DIGEST_CONSTANT = bytes([
    0x62, 0x7C, 0xB1, 0x80, 0x8A, 0xB9, 0x38, 0xE3,
    0x2C, 0x8C, 0x09, 0x17, 0x08, 0x72, 0x6A, 0x57,
    0x9E, 0x25, 0x86, 0xE4
])

# ============================================================
# ELF Parsing
# ============================================================

def parse_elf32_ehdr(data):
    fields = struct.unpack_from('<16s HHI III IHHHHHH', data, 0)
    return {
        'e_ident': fields[0], 'e_type': fields[1], 'e_machine': fields[2],
        'e_version': fields[3], 'e_entry': fields[4], 'e_phoff': fields[5],
        'e_shoff': fields[6], 'e_flags': fields[7], 'e_ehsize': fields[8],
        'e_phentsize': fields[9], 'e_phnum': fields[10], 'e_shentsize': fields[11],
        'e_shnum': fields[12], 'e_shstrndx': fields[13],
    }

def parse_elf32_phdrs(data, ehdr):
    phdrs = []
    for i in range(ehdr['e_phnum']):
        off = ehdr['e_phoff'] + i * ehdr['e_phentsize']
        f = struct.unpack_from('<IIIIIIII', data, off)
        phdrs.append({
            'p_type': f[0], 'p_offset': f[1], 'p_vaddr': f[2], 'p_paddr': f[3],
            'p_filesz': f[4], 'p_memsz': f[5], 'p_flags': f[6], 'p_align': f[7],
        })
    return phdrs

def parse_elf32_shdrs(data, ehdr):
    shdrs = []
    for i in range(ehdr['e_shnum']):
        off = ehdr['e_shoff'] + i * ehdr['e_shentsize']
        f = struct.unpack_from('<IIIIIIIIII', data, off)
        shdrs.append({
            'sh_name': f[0], 'sh_type': f[1], 'sh_flags': f[2], 'sh_addr': f[3],
            'sh_offset': f[4], 'sh_size': f[5], 'sh_link': f[6], 'sh_info': f[7],
            'sh_addralign': f[8], 'sh_entsize': f[9],
        })
    return shdrs

def strip_debug_and_fix_phdrs(velf_data, ehdr, phdrs, strip_symtab=False):
    """Strip DWARF debug sections and .rel.* relocations from the debug VELF copy.

    Official SDK SELFs have DWARF with consistent addressing. Our homebrew VELF
    has DWARF with absolute 0x81000XXX addresses but .symtab patched to
    segment-relative, which creates conflicts when Razor applies .rel.debug_*
    relocations using the patched symbol values.

    Also patches p_paddr to 0 in the embedded VELF's program headers.
    """
    shdrs = parse_elf32_shdrs(velf_data, ehdr)
    if not shdrs:
        return 0

    # Get section names
    shstrtab_sh = shdrs[ehdr['e_shstrndx']]
    shstrtab = bytes(velf_data[shstrtab_sh['sh_offset']:shstrtab_sh['sh_offset'] + shstrtab_sh['sh_size']])

    def sec_name(sh):
        idx = sh['sh_name']
        if idx >= len(shstrtab):
            return ''
        end = shstrtab.find(b'\x00', idx)
        return shstrtab[idx:end].decode('utf-8', errors='replace') if end > idx else ''

    # Zero out debug sections, standard relocations, and other noise.
    # The kernel uses the SCE .sce.rel section (SHT_SCE_RELA/PT_SCE_RELA),
    # not the standard .rel.* sections. Stripping standard relocations avoids
    # Razor applying them with our patched (segment-relative) symbol values.
    SHT_NULL = 0
    SHT_SYMTAB = 2
    SHT_STRTAB = 3
    SHT_REL = 9
    SHT_RELA = 4
    SHT_SCE_RELA = 0x60000000
    stripped = 0
    for i, sh in enumerate(shdrs):
        name = sec_name(sh)
        # Strip by name: debug, noise
        strip_by_name = (
            name.startswith('.debug_') or
            name == '.comment' or
            name == '.ARM.attributes' or
            name == '.ARM.exidx' or
            name == '.ARM.extab' or
            name == '.eh_frame' or
            name == '.tm_clone_table' or
            name == '.noinit'
        )
        # Strip by type: standard REL/RELA (but NOT SCE_RELA which the kernel needs)
        strip_by_type = sh['sh_type'] in (SHT_REL, SHT_RELA)
        # Optionally strip symtab/strtab for testing
        if strip_symtab and sh['sh_type'] == SHT_SYMTAB:
            strip_by_type = True
        if strip_symtab and sh['sh_type'] == SHT_STRTAB and name == '.strtab':
            strip_by_name = True
        if strip_by_name or strip_by_type:
            off = ehdr['e_shoff'] + i * ehdr['e_shentsize']
            struct.pack_into('<I', velf_data, off + 4, SHT_NULL)  # sh_type
            struct.pack_into('<I', velf_data, off + 20, 0)         # sh_size
            stripped += 1

    # Patch p_paddr to 0 in the VELF's own program headers
    for i in range(ehdr['e_phnum']):
        off = ehdr['e_phoff'] + i * ehdr['e_phentsize'] + 12  # p_paddr offset
        struct.pack_into('<I', velf_data, off, 0)

    return stripped


def make_symbols_relative(velf_data, ehdr, phdrs, max_symbols=0):
    """Convert absolute symbol addresses to section-relative offsets,
    strip non-essential symbols, and compact the symbol table.

    Official SDK SELFs use section-relative symbol values for ET_SCE_RELEXEC.
    Homebrew VELFs have absolute virtual addresses (0x81000XXX).
    Razor adds sh_addr to st_value to reconstruct the absolute address,
    so st_value must be relative to the section's sh_addr, not p_vaddr.

    Compacts the symtab by moving kept symbols to the front and shrinking
    sh_size, so Razor only sees the active entries.

    max_symbols: if > 0, cap the total active symbol count to avoid
    stack overflow in Razor's RecurseCallProfData (recursive call tree walker).
    Drops mapping symbols first, then smallest FUNC symbols.
    """
    shdrs = parse_elf32_shdrs(velf_data, ehdr)
    if not shdrs:
        return 0

    # find .symtab section and its index in the section header table
    SHT_SYMTAB = 2
    symtab = None
    symtab_idx = None
    for idx, sh in enumerate(shdrs):
        if sh['sh_type'] == SHT_SYMTAB:
            symtab = sh
            symtab_idx = idx
            break
    if not symtab or symtab['sh_entsize'] != 16:
        return 0

    # find .strtab (linked from .symtab via sh_link)
    strtab = shdrs[symtab['sh_link']]
    strtab_data = bytes(velf_data[strtab['sh_offset']:strtab['sh_offset'] + strtab['sh_size']])

    SHN_UNDEF = 0
    SHN_ABS = 0xFFF1
    SHN_COMMON = 0xFFF2

    STT_FUNC = 2

    # Max symbol name length
    MAX_NAME_LEN = 200

    num_syms = symtab['sh_size'] // 16

    # Phase 1: Collect kept symbols, patch values to section-relative
    kept_locals = []   # LOCAL symbols (binding 0) — must come first per ELF spec
    kept_globals = []  # GLOBAL/WEAK symbols
    stripped = 0
    truncated = 0
    patched = 0

    for i in range(num_syms):
        sym_off = symtab['sh_offset'] + i * 16
        st_name, st_value, st_size, st_info, st_other, st_shndx = \
            struct.unpack_from('<IIIBBH', velf_data, sym_off)

        # Always keep NULL symbol at index 0
        if i == 0:
            kept_locals.append((st_name, st_value, st_size, st_info, st_other, st_shndx))
            continue

        st_type = st_info & 0xF
        st_bind = st_info >> 4

        # Get the symbol name
        name = b''
        if st_name < len(strtab_data):
            end = strtab_data.find(b'\x00', st_name)
            name = strtab_data[st_name:end] if end > st_name else b''

        # Keep $t and $a mapping symbols (Razor needs them for Thumb/ARM mode)
        is_t_mapping = name.startswith(b'$t')
        is_a_mapping = name.startswith(b'$a')

        # Strip $d mapping symbols
        is_d_mapping = name in (b'$d', b'$d.0') or \
            (name.startswith(b'$d.') and len(name) <= 5)
        if is_d_mapping:
            stripped += 1
            continue

        # Only keep FUNC and $t/$a mapping symbols
        if st_type != STT_FUNC and not is_t_mapping and not is_a_mapping:
            stripped += 1
            continue

        # Truncate very long symbol names in .strtab
        if len(name) > MAX_NAME_LEN:
            trunc_off = strtab['sh_offset'] + st_name + MAX_NAME_LEN
            struct.pack_into('<B', velf_data, trunc_off, 0)
            truncated += 1

        # Make value section-relative
        new_value = st_value
        if st_shndx not in (SHN_UNDEF, SHN_ABS, SHN_COMMON) and st_shndx < len(shdrs):
            sec_addr = shdrs[st_shndx]['sh_addr']
            if sec_addr != 0 and st_value >= sec_addr:
                new_value = st_value - sec_addr
                patched += 1

        entry = (st_name, new_value, st_size, st_info, st_other, st_shndx)
        if st_bind == 0:  # STB_LOCAL
            kept_locals.append(entry)
        else:
            kept_globals.append(entry)

    # Phase 1.5: Fix zero-size symbols and deduplicate by address
    # This prevents Razor's address range linked list from corrupting:
    # - Zero-size symbols get inflated to fill remaining range, causing overlaps
    # - Multiple symbols at same address cause split/merge confusion
    def fix_and_dedup(syms, start_idx=0):
        """Fix zero-size FUNC symbols and deduplicate by address."""
        fixed_size = 0
        deduped = 0

        # First pass: fix zero-size FUNC symbols
        for i in range(start_idx, len(syms)):
            st_name, st_value, st_size, st_info, st_other, st_shndx = syms[i]
            st_type = st_info & 0xF
            if st_type == STT_FUNC and st_size == 0:
                # Use 2 bytes (minimum Thumb instruction size)
                syms[i] = (st_name, st_value, 2, st_info, st_other, st_shndx)
                fixed_size += 1

        # Second pass: deduplicate by (st_value, st_shndx) — keep largest
        seen = {}  # (value, shndx) -> index in syms
        remove = set()
        for i in range(start_idx, len(syms)):
            st_name, st_value, st_size, st_info, st_other, st_shndx = syms[i]
            key = (st_value, st_shndx)
            if key in seen:
                prev_idx = seen[key]
                prev_size = syms[prev_idx][2]
                if st_size > prev_size:
                    remove.add(prev_idx)
                    seen[key] = i
                else:
                    remove.add(i)
                deduped += 1
            else:
                seen[key] = i

        if remove:
            syms[:] = [s for i, s in enumerate(syms) if i not in remove]

        return fixed_size, deduped

    fixed_l, dedup_l = fix_and_dedup(kept_locals, start_idx=1)  # skip NULL
    fixed_g, dedup_g = fix_and_dedup(kept_globals)
    total_fixed = fixed_l + fixed_g
    total_dedup = dedup_l + dedup_g
    if total_fixed:
        print(f"  Fixed {total_fixed} zero-size FUNC symbols (set to 2 bytes)")
    if total_dedup:
        print(f"  Deduplicated {total_dedup} symbols sharing same address")

    # Phase 1.75: Cap FUNC symbol count to avoid stack overflow in Razor's
    # RecurseCallProfData and m_vecCallees walker. Too many FUNC symbols =
    # too many function transitions in trace = deep/cyclic call tree = crash.
    # KEEP all $t/$a mapping symbols (Razor needs them for ARM/Thumb decoding).
    if max_symbols > 0:
        # Separate mapping and FUNC symbols (keep NULL at locals[0])
        func_locals = []
        mapping_locals = []
        for i, entry in enumerate(kept_locals):
            if i == 0:
                continue
            st_name_val = entry[0]
            name = b''
            if st_name_val < len(strtab_data):
                end = strtab_data.find(b'\x00', st_name_val)
                name = strtab_data[st_name_val:end] if end > st_name_val else b''
            if name.startswith(b'$t') or name.startswith(b'$a'):
                mapping_locals.append(entry)
            else:
                func_locals.append(entry)

        func_globals = []
        mapping_globals = []
        for entry in kept_globals:
            st_name_val = entry[0]
            name = b''
            if st_name_val < len(strtab_data):
                end = strtab_data.find(b'\x00', st_name_val)
                name = strtab_data[st_name_val:end] if end > st_name_val else b''
            if name.startswith(b'$t') or name.startswith(b'$a'):
                mapping_globals.append(entry)
            else:
                func_globals.append(entry)

        num_func = len(func_locals) + len(func_globals)
        num_mapping = len(mapping_locals) + len(mapping_globals)
        dropped_func = 0

        if num_func > max_symbols:
            # Sort all FUNC by size descending, keep the largest
            all_func = func_locals + func_globals
            all_func.sort(key=lambda s: s[2], reverse=True)
            dropped_func = num_func - max_symbols
            all_func = all_func[:max_symbols]
            # Re-split into locals/globals
            func_locals = []
            func_globals = []
            for entry in all_func:
                st_bind = entry[3] >> 4
                if st_bind == 0:
                    func_locals.append(entry)
                else:
                    func_globals.append(entry)

        # Reassemble: NULL + mapping_locals + func_locals, mapping_globals + func_globals
        kept_locals = [kept_locals[0]] + mapping_locals + func_locals
        kept_globals = mapping_globals + func_globals
        new_total = len(kept_locals) + len(kept_globals) - 1
        print(f"  Symbol cap: {num_func} FUNC -> {num_func - dropped_func} (max {max_symbols}), "
              f"{num_mapping} mapping kept, {new_total} total")
        if dropped_func:
            print(f"    Dropped {dropped_func} smallest FUNC symbols")

    # Phase 2: Write compacted symtab — locals first, then globals (ELF requirement)
    all_kept = kept_locals + kept_globals
    for i, (st_name, st_value, st_size, st_info, st_other, st_shndx) in enumerate(all_kept):
        sym_off = symtab['sh_offset'] + i * 16
        struct.pack_into('<IIIBBH', velf_data, sym_off,
                         st_name, st_value, st_size, st_info, st_other, st_shndx)

    # Zero out remaining entries
    for i in range(len(all_kept), num_syms):
        sym_off = symtab['sh_offset'] + i * 16
        struct.pack_into('<IIIBBH', velf_data, sym_off, 0, 0, 0, 0, 0, 0)

    # Don't change sh_size or sh_info — Razor validates the debug SELF against
    # the running module and rejects it if section metadata differs.

    print(f"  Symtab compacted: {len(all_kept)} active entries "
          f"({len(kept_locals)} local + {len(kept_globals)} global), "
          f"{num_syms - len(all_kept)} zeroed at end")

    return patched, stripped, truncated

def inject_so_symbols(velf_data, ehdr, so_modules, max_so_symbols=0):
    """Inject .so module FUNC symbols into the VELF's symtab.

    Razor's global address tree (FUN_100ac8a0) can resolve any address
    regardless of module code section ranges. By injecting .so symbols
    into the bgda symtab, Razor can track function transitions across
    the bgda/so boundary, producing a correct call tree.

    so_modules: list of (path, load_address) tuples
    max_so_symbols: if > 0, limit total injected .so symbols (largest first)
    """
    shdrs = parse_elf32_shdrs(velf_data, ehdr)
    if not shdrs:
        return 0

    # Find .symtab
    symtab = None
    for sh in shdrs:
        if sh['sh_type'] == 2:  # SHT_SYMTAB
            symtab = sh
            break
    if not symtab or symtab['sh_entsize'] != 16:
        return 0

    # Find .strtab
    strtab_sh = shdrs[symtab['sh_link']]
    strtab_end_used = strtab_sh['sh_offset']  # We'll scan to find actual end

    # Find the code section index (first section with SHF_EXECINSTR)
    code_shndx = 1  # Default to section 1 (.text)
    code_sh_addr = 0
    SHF_EXECINSTR = 0x4
    for idx, sh in enumerate(shdrs):
        if sh['sh_flags'] & SHF_EXECINSTR and sh['sh_type'] == 1:  # SHT_PROGBITS
            code_shndx = idx
            code_sh_addr = sh['sh_addr']
            break

    num_syms = symtab['sh_size'] // 16

    # Find first zeroed slot (after compacted symbols)
    first_free = 0
    for i in range(num_syms):
        sym_off = symtab['sh_offset'] + i * 16
        st_info = struct.unpack_from('<B', velf_data, sym_off + 12)[0]
        st_name = struct.unpack_from('<I', velf_data, sym_off)[0]
        st_value = struct.unpack_from('<I', velf_data, sym_off + 4)[0]
        if st_info == 0 and st_name == 0 and st_value == 0 and i > 0:
            first_free = i
            break
    if first_free == 0:
        print("  No free symtab slots for .so injection")
        return 0

    available_slots = num_syms - first_free

    # Find orphaned strtab space: scan backwards from end to find
    # writable area. We'll write .so names starting from the end of
    # the last referenced string.
    # Strategy: find max st_name offset among active symbols to know
    # where "used" strtab ends, then write new names after that.
    strtab_data = velf_data[strtab_sh['sh_offset']:strtab_sh['sh_offset'] + strtab_sh['sh_size']]
    max_name_end = 0
    for i in range(first_free):
        sym_off = symtab['sh_offset'] + i * 16
        st_name = struct.unpack_from('<I', velf_data, sym_off)[0]
        if st_name < strtab_sh['sh_size']:
            end = strtab_data.find(b'\x00', st_name)
            if end > max_name_end:
                max_name_end = end + 1  # past the null terminator
    # Also check stripped symbols' names — they're still referenced by the
    # original (now-zeroed) entries but the strtab data is still there.
    # Safe to overwrite anything past max_name_end.
    strtab_write_pos = max_name_end
    strtab_capacity = strtab_sh['sh_size'] - strtab_write_pos

    print(f"  .so injection: {available_slots} free symtab slots, "
          f"{strtab_capacity} strtab bytes available (from offset {strtab_write_pos})")

    # Parse each .so and collect ALL FUNC symbols first
    STT_FUNC = 2
    STB_GLOBAL = 1
    all_so_syms = []  # (abs_addr, size, name_bytes, so_basename)

    for so_path, load_addr in so_modules:
        if not os.path.exists(so_path):
            print(f"    {os.path.basename(so_path)}: NOT FOUND, skipping")
            continue

        with open(so_path, 'rb') as f:
            so_data = f.read()

        if so_data[0:4] != b'\x7fELF':
            print(f"    {os.path.basename(so_path)}: not ELF, skipping")
            continue

        so_ehdr = parse_elf32_ehdr(so_data)
        so_shdrs = parse_elf32_shdrs(so_data, so_ehdr)

        # Find .dynsym
        so_dynsym = None
        for sh in so_shdrs:
            if sh['sh_type'] == 11:  # SHT_DYNSYM
                so_dynsym = sh
                break
        if not so_dynsym:
            print(f"    {os.path.basename(so_path)}: no .dynsym, skipping")
            continue

        so_strtab_sh = so_shdrs[so_dynsym['sh_link']]
        so_strtab = so_data[so_strtab_sh['sh_offset']:so_strtab_sh['sh_offset'] + so_strtab_sh['sh_size']]

        entsize = so_dynsym['sh_entsize'] or 16
        so_num = so_dynsym['sh_size'] // entsize
        count = 0

        for j in range(so_num):
            sym_off = so_dynsym['sh_offset'] + j * entsize
            st_name_v, st_value, st_size_val, st_info, st_other, st_shndx = \
                struct.unpack_from('<IIIBBH', so_data, sym_off)
            if (st_info & 0xF) != STT_FUNC or st_value == 0:
                continue

            abs_addr = load_addr + st_value
            so_name = b''
            if st_name_v < len(so_strtab):
                end = so_strtab.find(b'\x00', st_name_v)
                so_name = so_strtab[st_name_v:end] if end > st_name_v else b''
            if len(so_name) > 120:
                so_name = so_name[:120]

            all_so_syms.append((abs_addr, st_size_val, so_name, os.path.basename(so_path)))
            count += 1

        print(f"    {os.path.basename(so_path):25s} @ 0x{load_addr:08x}: {count} FUNC symbols found")

    # Apply max_so_symbols limit (keep largest)
    if max_so_symbols > 0 and len(all_so_syms) > max_so_symbols:
        all_so_syms.sort(key=lambda s: s[1], reverse=True)
        dropped = len(all_so_syms) - max_so_symbols
        all_so_syms = all_so_syms[:max_so_symbols]
        print(f"    Capped to {max_so_symbols} largest .so symbols (dropped {dropped})")

    # Write into symtab slots
    slot_idx = first_free
    total_injected = 0
    for abs_addr, st_size_val, so_name, _ in all_so_syms:
        if slot_idx >= num_syms:
            break

        relative_val = abs_addr - code_sh_addr

        # Write name into bgda strtab if space available
        name_offset = 0
        if so_name and strtab_write_pos + len(so_name) + 1 <= strtab_sh['sh_size']:
            name_offset = strtab_write_pos
            write_pos = strtab_sh['sh_offset'] + strtab_write_pos
            velf_data[write_pos:write_pos + len(so_name)] = so_name
            velf_data[write_pos + len(so_name)] = 0
            strtab_write_pos += len(so_name) + 1

        out_off = symtab['sh_offset'] + slot_idx * 16
        st_info_out = (STB_GLOBAL << 4) | STT_FUNC
        struct.pack_into('<IIIBBH', velf_data, out_off,
                         name_offset, relative_val, st_size_val,
                         st_info_out, 0, code_shndx)
        slot_idx += 1
        total_injected += 1

    print(f"  Total .so symbols injected: {total_injected}")
    return total_injected


# ============================================================
# Crypto (matches vita-make-fself.c)
# ============================================================

def sha256_32_file(data):
    hash1 = hashlib.sha256(data).digest()
    hash2 = hashlib.sha256(hash1).digest()
    return (hash2[0] << 24) | (hash2[1] << 16) | (hash2[2] << 8) | hash2[3]

def patch_module_nid(velf_data, ehdr, mod_nid):
    e_type = ehdr['e_type']
    e_entry = ehdr['e_entry']
    if e_type == ET_SCE_RELEXEC:
        seg = e_entry >> 30
        off = e_entry & 0x3FFFFFFF
        phdr_off = ehdr['e_phoff'] + seg * ehdr['e_phentsize']
        p_offset = struct.unpack_from('<I', velf_data, phdr_off + 4)[0]
        info_offset = p_offset + off
    elif e_type == ET_SCE_EXEC:
        phdr_off = ehdr['e_phoff']
        p_offset = struct.unpack_from('<I', velf_data, phdr_off + 4)[0]
        p_paddr = struct.unpack_from('<I', velf_data, phdr_off + 12)[0]
        info_offset = p_offset + p_paddr
    else:
        return
    nid_offset = info_offset + 0x34
    struct.pack_into('<I', velf_data, nid_offset, mod_nid)

def align_up(val, alignment):
    return (val + alignment - 1) & ~(alignment - 1)

# ============================================================
# Build SELF header (shared between compressed and uncompressed)
# ============================================================

def build_self_header(ehdr, phdrs, elf_digest, elf_filesize, seg_entries, extra_phnum=0):
    """Build the 0x1000-byte SELF header.

    seg_entries: list of (self_offset, length, compression, encryption) per phdr
    """
    phnum = ehdr['e_phnum'] + extra_phnum

    phdr_off_self = 0xE0
    seg_info_off = align_up(phdr_off_self + phnum * 32, 0x10)
    scever_off = seg_info_off + phnum * 32
    ctrl_off = scever_off + 16

    header_end = ctrl_off + CTRL_TOTAL_SIZE
    assert header_end <= HEADER_LEN, f"Header overflow: 0x{header_end:x}"

    output = bytearray(HEADER_LEN)

    # --- SCE Header ---
    struct.pack_into('<I', output, 0x00, 0x00454353)       # magic
    struct.pack_into('<I', output, 0x04, 3)                 # version
    struct.pack_into('<H', output, 0x08, 0x00C0)            # sdk_type
    struct.pack_into('<H', output, 0x0A, 1)                 # header_type
    struct.pack_into('<I', output, 0x0C, 0x600)             # metadata_offset
    struct.pack_into('<Q', output, 0x10, HEADER_LEN)        # header_len
    struct.pack_into('<Q', output, 0x18, elf_filesize)      # elf_filesize
    # 0x20: self_filesize - set by caller
    struct.pack_into('<Q', output, 0x28, 0)                 # unknown
    struct.pack_into('<Q', output, 0x30, 4)                 # self_offset
    struct.pack_into('<Q', output, 0x38, 0x80)              # appinfo_offset
    struct.pack_into('<Q', output, 0x40, 0xA0)              # elf_offset
    struct.pack_into('<Q', output, 0x48, phdr_off_self)     # phdr_offset
    struct.pack_into('<Q', output, 0x50, HEADER_LEN + ehdr['e_shoff'])  # shdr_offset
    struct.pack_into('<Q', output, 0x58, seg_info_off)      # section_info_offset
    struct.pack_into('<Q', output, 0x60, scever_off)        # sceversion_offset
    struct.pack_into('<Q', output, 0x68, ctrl_off)          # controlinfo_offset
    struct.pack_into('<Q', output, 0x70, CTRL_TOTAL_SIZE)   # controlinfo_size
    struct.pack_into('<Q', output, 0x78, 0)                 # padding (must be 0)

    # --- AppInfo ---
    struct.pack_into('<Q', output, 0x80, 0x2F00000000000002)
    struct.pack_into('<I', output, 0x88, 0)
    struct.pack_into('<I', output, 0x8C, 8)
    struct.pack_into('<Q', output, 0x90, 0x0001000000000000)
    struct.pack_into('<Q', output, 0x98, 0)

    # --- ELF Header ---
    struct.pack_into('<I', output, 0xA0, 0x464C457F)        # magic
    output[0xA4:0xA8] = b'\x01\x01\x01\x00'                 # class/data/version/os
    struct.pack_into('<H', output, 0xB0, ehdr['e_type'])
    struct.pack_into('<H', output, 0xB2, ehdr['e_machine'])
    struct.pack_into('<I', output, 0xB4, ehdr['e_version'])
    struct.pack_into('<I', output, 0xB8, ehdr['e_entry'])
    struct.pack_into('<I', output, 0xBC, 0x34)               # e_phoff
    struct.pack_into('<I', output, 0xC0, ehdr['e_shoff'])    # e_shoff preserved
    struct.pack_into('<I', output, 0xC4, ehdr['e_flags'] & ~0x200)
    struct.pack_into('<H', output, 0xC8, 52)                 # e_ehsize
    struct.pack_into('<H', output, 0xCA, 32)                 # e_phentsize
    struct.pack_into('<H', output, 0xCC, phnum)
    struct.pack_into('<H', output, 0xCE, 40)                 # e_shentsize
    struct.pack_into('<H', output, 0xD0, ehdr['e_shnum'])    # preserved
    struct.pack_into('<H', output, 0xD2, ehdr['e_shstrndx']) # preserved

    # --- Program Headers (with p_paddr = 0, matching official SELF) ---
    for i, phdr in enumerate(phdrs):
        off = phdr_off_self + i * 32
        struct.pack_into('<IIIIIIII', output, off,
            phdr['p_type'],
            phdr['p_offset'],
            phdr['p_vaddr'],
            0,                              # p_paddr = 0 (critical fix!)
            phdr['p_filesz'],
            phdr['p_memsz'],
            phdr['p_flags'],
            min(phdr['p_align'], 0x1000))

    # --- Segment Info ---
    for i, seg in enumerate(seg_entries):
        off = seg_info_off + i * 32
        struct.pack_into('<QQQQ', output, off, seg[0], seg[1], seg[2], seg[3])

    # --- SCE Version ---
    struct.pack_into('<IIII', output, scever_off, 1, 0, 16, 0)

    # --- Control Info (types 4, 5, 6, 7) ---
    c4_off = ctrl_off
    struct.pack_into('<III', output, c4_off, 4, CTRL4_SIZE, 1)
    output[c4_off+0x10:c4_off+0x10+0x14] = DIGEST_CONSTANT
    output[c4_off+0x24:c4_off+0x24+0x20] = elf_digest

    c5_off = c4_off + CTRL4_SIZE
    struct.pack_into('<III', output, c5_off, 5, CTRL5_SIZE, 1)

    c6_off = c5_off + CTRL5_SIZE
    struct.pack_into('<III', output, c6_off, 6, CTRL6_SIZE, 1)
    struct.pack_into('<I', output, c6_off + 16, 1)  # is_used

    c7_off = c6_off + CTRL6_SIZE
    struct.pack_into('<II', output, c7_off, 7, CTRL7_SIZE)

    return output


def make_fself_debug(input_path, output_path, strip_symtab=False, max_symbols=0, so_modules=None, max_so_symbols=0):
    with open(input_path, 'rb') as f:
        velf_data = bytearray(f.read())

    if velf_data[0:4] != b'\x7fELF':
        print(f"Error: {input_path} is not an ELF file", file=sys.stderr)
        return 1

    ehdr = parse_elf32_ehdr(velf_data)
    phdrs = parse_elf32_phdrs(velf_data, ehdr)

    print(f"Input VELF: {len(velf_data)} bytes")
    print(f"  e_type=0x{ehdr['e_type']:04x}, e_entry=0x{ehdr['e_entry']:x}")
    print(f"  {ehdr['e_phnum']} phdrs, {ehdr['e_shnum']} shdrs")
    for i, p in enumerate(phdrs):
        ptype = {1:'PT_LOAD', 0x60000000:'PT_SCE_RELA'}.get(p['p_type'], f"0x{p['p_type']:x}")
        print(f"  phdr[{i}]: {ptype} vaddr=0x{p['p_vaddr']:x} paddr=0x{p['p_paddr']:x} off=0x{p['p_offset']:x} fsz=0x{p['p_filesz']:x}")

    # Patch module_nid and compute digest (on ORIGINAL velf_data)
    mod_nid = sha256_32_file(bytes(velf_data))
    patch_module_nid(velf_data, ehdr, mod_nid)
    elf_digest = hashlib.sha256(bytes(velf_data)).digest()
    print(f"  module_nid=0x{mod_nid:08x}")
    print(f"  elf_digest={elf_digest.hex()}")

    elf_filesize = len(velf_data)

    # ========================================
    # Version 1: Compressed (guaranteed to load)
    # Uses original velf_data (absolute symbol addresses)
    # ========================================
    seg_entries_comp = []
    compressed_data = bytearray()
    cur_off = HEADER_LEN
    for phdr in phdrs:
        raw = bytes(velf_data[phdr['p_offset']:phdr['p_offset'] + phdr['p_filesz']])
        comp = zlib.compress(raw, 6)
        seg_entries_comp.append((cur_off, len(comp), 2, 2))
        if len(compressed_data) < cur_off - HEADER_LEN:
            compressed_data += b'\x00' * (cur_off - HEADER_LEN - len(compressed_data))
        compressed_data += comp
        cur_off = align_up(cur_off + len(comp), 0x10)

    header_comp = build_self_header(ehdr, phdrs, elf_digest, elf_filesize, seg_entries_comp)
    # For compressed: set shdr_offset=0, e_shnum=0 (no readable section data)
    struct.pack_into('<Q', header_comp, 0x50, 0)    # shdr_offset = 0
    struct.pack_into('<H', header_comp, 0xD0, 0)    # e_shnum = 0
    struct.pack_into('<H', header_comp, 0xD2, 0)    # e_shstrndx = 0

    self_comp = header_comp + compressed_data
    while len(self_comp) % 0x10:
        self_comp += b'\x00'
    struct.pack_into('<Q', self_comp, 0x20, len(self_comp))

    with open(output_path, 'wb') as f:
        f.write(self_comp)
    print(f"\nCompressed: {output_path} ({len(self_comp)} bytes)")

    # ========================================
    # Version 2: Uncompressed (for Razor)
    # Uses a copy with segment-relative symbol addresses
    # ========================================
    debug_velf = bytearray(velf_data)
    if not strip_symtab:
        patched, stripped, truncated = make_symbols_relative(debug_velf, ehdr, phdrs, max_symbols=max_symbols)
        print(f"\nSymbol fixup: {patched} symbols made relative, {stripped} symbols stripped, {truncated} names truncated")
    else:
        print(f"\n--nosym: skipping symbol fixup, will strip .symtab/.strtab")
    # Inject .so module symbols into the symtab's zeroed slots
    if so_modules and not strip_symtab:
        print(f"\nInjecting .so module symbols:")
        inject_so_symbols(debug_velf, ehdr, so_modules, max_so_symbols=max_so_symbols)

    sections_stripped = strip_debug_and_fix_phdrs(debug_velf, ehdr, phdrs, strip_symtab=strip_symtab)
    print(f"Stripped {sections_stripped} debug/noise sections, patched embedded VELF phdrs")

    seg_entries_raw = []
    for phdr in phdrs:
        seg_entries_raw.append((
            HEADER_LEN + phdr['p_offset'],  # offset in SELF
            phdr['p_filesz'],                # length
            1,                               # compression: uncompressed
            2,                               # encryption: plain
        ))

    header_raw = build_self_header(ehdr, phdrs, elf_digest, elf_filesize, seg_entries_raw)
    self_raw = header_raw + debug_velf
    struct.pack_into('<Q', self_raw, 0x20, len(self_raw))

    base, ext = os.path.splitext(output_path)
    debug_path = base + '_debug' + ext
    with open(debug_path, 'wb') as f:
        f.write(self_raw)
    print(f"Uncompressed: {debug_path} ({len(self_raw)} bytes)")

    # ========================================
    # Verify
    # ========================================
    print(f"\nKey fields (uncompressed debug):")
    print(f"  elf_filesize=0x{elf_filesize:x}")
    print(f"  self_filesize=0x{len(self_raw):x}")
    print(f"  shdr_offset=0x{struct.unpack_from('<Q', self_raw, 0x50)[0]:x}")
    print(f"  e_shnum={struct.unpack_from('<H', self_raw, 0xD0)[0]}")
    print(f"  p_paddr[0]=0x{struct.unpack_from('<I', self_raw, 0xE0+12)[0]:x} (should be 0!)")
    si = struct.unpack_from('<QQQQ', self_raw, struct.unpack_from('<Q', self_raw, 0x58)[0])
    print(f"  seg[0]: off=0x{si[0]:x} len=0x{si[1]:x} comp={si[2]} enc={si[3]}")

    return 0


def main():
    strip_symtab = '--nosym' in sys.argv
    max_symbols = 0
    max_so_symbols = 0
    so_modules = []
    argv_filtered = []
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == '--nosym':
            pass
        elif sys.argv[i] == '--max-symbols' and i + 1 < len(sys.argv):
            max_symbols = int(sys.argv[i + 1])
            i += 1
        elif sys.argv[i].startswith('--max-symbols='):
            max_symbols = int(sys.argv[i].split('=', 1)[1])
        elif sys.argv[i] == '--max-so-symbols' and i + 1 < len(sys.argv):
            max_so_symbols = int(sys.argv[i + 1])
            i += 1
        elif sys.argv[i].startswith('--max-so-symbols='):
            max_so_symbols = int(sys.argv[i].split('=', 1)[1])
        elif sys.argv[i] == '--so' and i + 1 < len(sys.argv):
            # Format: --so "path/to/lib.so@0xADDRESS"
            spec = sys.argv[i + 1]
            if '@' in spec:
                path, addr_str = spec.rsplit('@', 1)
                so_modules.append((path, int(addr_str, 16)))
            else:
                print(f"Error: --so format must be 'path@0xADDRESS', got: {spec}", file=sys.stderr)
                sys.exit(1)
            i += 1
        else:
            argv_filtered.append(sys.argv[i])
        i += 1
    if len(argv_filtered) < 2:
        print(f"Usage: {sys.argv[0]} [--nosym] [--max-symbols N] [--so path@0xADDR ...] input.velf output.self",
              file=sys.stderr)
        sys.exit(1)
    rc = make_fself_debug(argv_filtered[0], argv_filtered[1],
                          strip_symtab=strip_symtab, max_symbols=max_symbols,
                          so_modules=so_modules or None, max_so_symbols=max_so_symbols)
    sys.exit(rc)

if __name__ == '__main__':
    main()
