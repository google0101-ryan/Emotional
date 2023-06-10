// (c) Copyright 2022-2023 Ryan Ilari
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#include <cstdint>

namespace CDVD
{
	uint8_t ReadNStatus();
	uint8_t ReadSStatus();

	void AddSCommand(uint8_t cmd);
	uint8_t ReadSCommand();

	uint8_t ReadSResult();
}
