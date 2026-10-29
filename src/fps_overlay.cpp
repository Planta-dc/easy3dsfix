/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#ifdef __3DS__
#include <3ds.h>
#endif

#include "fps_overlay.h"
#include "game_clock.h"
#include "bitmap.h"
#include "options.h"
#include "utils.h"
#include "input.h"
#include "font.h"
#include "drawable_mgr.h"
#ifdef __3DS__
#include "player.h"
#endif

using namespace std::chrono_literals;

static constexpr auto refresh_frequency = 1s;

FpsOverlay::FpsOverlay() :
	Drawable(Priority_Overlay + 100, Drawable::Flags::Global)
{
	DrawableMgr::Register(this);

	UpdateText();
}

void FpsOverlay::UpdateText() {
	auto fps = Utils::RoundTo<int>(Game_Clock::GetFPS());
	text = fmt::format("FPS: {}", fps);

	if (Game_Clock::GetTargetGameFps() != DEFAULT_FPS) {
		text += fmt::format(" | TPS: {}", Game_Clock::GetTargetGameFps());
	}

	fps_dirty = true;
#ifdef __3DS__
	if (draw_performance_monitor) {
		const double app_used_mb = osGetMemRegionUsed(MEMREGION_APPLICATION) / (1024.0 * 1024.0);
		const double app_total_mb = osGetMemRegionSize(MEMREGION_APPLICATION) / (1024.0 * 1024.0);
		double frame_ms = 0.0;
		double update_ms = 0.0;
		double draw_ms = 0.0;
		Player::GetPerformanceFrameTimes(frame_ms, update_ms, draw_ms);
		performance_text = fmt::format("RAM {:.0f}/{:.0f}MB | F {:.0f}ms U {:.0f} D {:.0f}ms",
			app_used_mb, app_total_mb, frame_ms, update_ms, draw_ms);
		performance_dirty = true;
	}
#endif
}

bool FpsOverlay::Update() {
	int mod = static_cast<int>(Game_Clock::GetGameSpeedFactor());
	if (mod != last_speed_mod) {
		speedup_dirty = true;
		last_speed_mod = mod;
	}

	auto now = Game_Clock::GetFrameTime();
	auto dt = now - last_refresh_time;
	if (dt < refresh_frequency) {
		return false;
	}
	last_refresh_time = now;

	UpdateText();

	return true;
}

void FpsOverlay::Draw(Bitmap& dst) {
	if (draw_fps) {
		if (fps_dirty) {
			std::string text = GetFpsString();
			Rect rect = Text::GetSize(*Font::DefaultBitmapFont(), text);

			if (!fps_bitmap || fps_bitmap->GetWidth() < rect.width + 1) {
				// Height never changes
				fps_bitmap = Bitmap::Create(rect.width + 1, rect.height - 1, true);
			}
			fps_bitmap->Clear();
			fps_bitmap->Fill(Color(0, 0, 0, 128));
			Text::Draw(*fps_bitmap, 1, 0, *Font::DefaultBitmapFont(), Color(255, 255, 255, 255), text);

			fps_rect = Rect(0, 0, rect.width + 1, rect.height - 1);

			fps_dirty = false;
		}

		dst.Blit(1, 2, *fps_bitmap, fps_rect, 255);
	}
#ifdef __3DS__
	if (draw_performance_monitor) {
		if (performance_dirty) {
			Rect rect = Text::GetSize(*Font::DefaultBitmapFont(), performance_text);
			if (!performance_bitmap || performance_bitmap->GetWidth() < rect.width + 2 || performance_bitmap->GetHeight() < rect.height) {
				performance_bitmap = Bitmap::Create(rect.width + 2, rect.height, true);
			}
			performance_bitmap->Clear();
			performance_bitmap->Fill(Color(0, 0, 0, 160));
			Text::Draw(*performance_bitmap, 1, 0, *Font::DefaultBitmapFont(), Color(255, 255, 255, 255), performance_text);
			performance_rect = Rect(0, 0, rect.width + 2, rect.height);
			performance_dirty = false;
		}
		const int y = draw_fps ? 2 + fps_rect.height + 2 : 2;
		dst.Blit(1, y, *performance_bitmap, performance_rect, 255);
	}
#endif

	// Always drawn when speedup is on independent of FPS
	if (last_speed_mod > 1) {
		if (speedup_dirty) {
			std::string text = "> x" + std::to_string(last_speed_mod);

			Rect rect = Text::GetSize(*Font::DefaultBitmapFont(), text);

			if (!speedup_bitmap || speedup_bitmap->GetWidth() < rect.width + 1) {
				// Height never changes
				speedup_bitmap = Bitmap::Create(rect.width + 1, rect.height - 1, true);
			}
			speedup_bitmap->Clear();
			speedup_bitmap->Fill(Color(0, 0, 0, 128));
			Text::Draw(*speedup_bitmap, 1, 0, *Font::DefaultBitmapFont(), Color(255, 255, 255, 255), text);

			speedup_rect = Rect(0, 0, rect.width + 1, rect.height - 1);

			speedup_dirty = false;
		}

		int dwidth = dst.GetWidth();
		dst.Blit(dwidth - speedup_rect.width - 1, 2, *speedup_bitmap, speedup_rect, 255);
	}
}

