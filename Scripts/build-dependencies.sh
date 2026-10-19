#!/usr/bin/env bash
# Copyright (c) 2026 DLR e.V.
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="${IMAGE_TAG:-gemstar:esmini_thirdparty}"
THIRD_PARTY_DIR="$ROOT_DIR/Source/ue_esmini_featureRuntime/ThirdParty/esmini"
INCLUDE_DIR="$THIRD_PARTY_DIR/Include"
LIB_DIR="$THIRD_PARTY_DIR/Lib"

docker build -t "$IMAGE_TAG" "$ROOT_DIR"

container_id="$(docker create "$IMAGE_TAG")"
cleanup() {
    docker rm -f "$container_id" >/dev/null 2>&1 || true
}
trap cleanup EXIT

mkdir -p "$INCLUDE_DIR" "$LIB_DIR"

docker cp "$container_id:/home/ue4/UnrealEngine/esmini/EnvironmentSimulator/Libraries/esminiLib/esminiLib.hpp" "$INCLUDE_DIR/"
docker cp "$container_id:/home/ue4/UnrealEngine/esmini/EnvironmentSimulator/Libraries/esminiRMLib/esminiRMLib.hpp" "$INCLUDE_DIR/"
docker cp "$container_id:/home/ue4/UnrealEngine/esmini/build/EnvironmentSimulator/Libraries/esminiLib/libesminiLib_combined.a" "$LIB_DIR/libesminiLibs_static.a"

printf 'Built %s and refreshed Linux third-party artefacts.\n' "$IMAGE_TAG"
