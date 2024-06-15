#pragma once

#include "CorradeOptional.h"

#include "whalECS/src/Expected.h"

Corrade::Containers::Optional<Error> loadDebugScene();
Corrade::Containers::Optional<Error> loadTestMap();
