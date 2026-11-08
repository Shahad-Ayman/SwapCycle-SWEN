#pragma once

// UUID generation and validation.

#include <string>
#include <string_view>

namespace swapcycle::common {

// A random (version 4) UUID in lowercase canonical form, e.g.
// "3f2b8c1e-9a4d-4e6f-8b2a-1c3d5e7f9a0b". Uses libsodium's CSPRNG, so it is
// safe for upload file names and other values that must not be guessable.
std::string generateUuid();

// True for the canonical 8-4-4-4-12 hex form (either case). Use it on path
// parameters before they reach a query, so bad IDs become a clean 404/400
// instead of a database error.
bool isValidUuid(std::string_view text);

}  // namespace swapcycle::common
