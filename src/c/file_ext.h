#pragma once

#include <stdbool.h>
#include <wchar.h>

/**
 * @brief Case-insensitive ASCII comparison for extension strings
 *
 * Compares two wide-character strings using case-insensitive ASCII comparison.
 * Only ASCII characters (A-Z) are case-folded; other characters must match exactly.
 *
 * @param ext1 First extension string (e.g., L".txt" or pointer to extension in path)
 * @param ext2 Second extension string to compare against
 * @return true if strings match (case-insensitive), false otherwise
 *
 * @note Returns false if either ext1 or ext2 is NULL
 * @note Only ASCII uppercase letters (A-Z) are converted to lowercase
 * @note This function compares the entire strings, not just suffixes
 *
 * @example
 *   extension_equals(L".TXT", L".txt")  // returns true
 *   extension_equals(L".txt", L".TXT")  // returns true
 *   extension_equals(L".doc", L".txt")  // returns false
 *   extension_equals(NULL, L".txt")     // returns false
 */
static inline bool gcmz_extension_equals(wchar_t const *ext1, wchar_t const *ext2) {
  if (!ext1 || !ext2) {
    return false;
  }

  for (; *ext1 != L'\0' && *ext2 != L'\0'; ++ext1, ++ext2) {
    wchar_t c1 = *ext1;
    wchar_t c2 = *ext2;

    // Convert to lowercase if ASCII uppercase
    if (c1 >= L'A' && c1 <= L'Z') {
      c1 |= 0x20;
    }
    if (c2 >= L'A' && c2 <= L'Z') {
      c2 |= 0x20;
    }

    if (c1 != c2) {
      return false;
    }
  }

  // Both strings must end at the same time
  return *ext1 == L'\0' && *ext2 == L'\0';
}
