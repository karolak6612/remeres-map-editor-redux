#include "ui/theme.h"

Theme::Type Theme::current_type = Theme::Type::System;

void Theme::SetType(Theme::Type type) {
	current_type = type;
}

Theme::Type Theme::GetType() {
	return current_type;
}

wxColour Theme::Get(Role role) {
	if (current_type == Type::Dark) {
		return GetDark(role);
	} else if (current_type == Type::Light) {
		return GetLight(role);
	} else {
		// System
		if (wxSystemSettings::GetAppearance().IsDark()) {
			return GetDark(role);
		} else {
			return GetLight(role);
		}
	}
}

wxColour Theme::GetDark(Role role) {
	switch (role) {
		case Role::Surface:
			return wxColour(45, 45, 48);
		case Role::Background:
			return wxColour(30, 30, 30);
		case Role::Header:
			return wxColour(25, 25, 25);
		case Role::Accent:
			return wxColour(0, 120, 215);
		case Role::AccentHover:
			return wxColour(0, 150, 255);
		case Role::Text:
			return wxColour(230, 230, 230);
		case Role::TextSubtle:
			return wxColour(150, 150, 150);
		case Role::TextOnAccent:
			return wxColour(255, 255, 255);
		case Role::Border:
			return wxColour(60, 60, 60);
		case Role::Selected:
			return wxColour(0, 120, 215, 60); // Alpha translucency
		case Role::Error:
			return wxColour(200, 50, 50);
		case Role::CardBase:
			return wxColour(50, 50, 55);
		case Role::CardBaseHover:
			return wxColour(60, 60, 65);
		case Role::CardBorder:
			return wxColour(80, 80, 80);
		default:
			return *wxWHITE;
	}
}

wxColour Theme::GetLight(Role role) {
	switch (role) {
		case Role::Surface:
			return wxColour(240, 240, 240); // Standard dialog background
		case Role::Background:
			return *wxWHITE; // Lists, entries
		case Role::Header:
			return wxColour(220, 220, 220);
		case Role::Accent:
			return wxColour(0, 120, 215); // Keep blue
		case Role::AccentHover:
			return wxColour(0, 100, 180);
		case Role::Text:
			return *wxBLACK;
		case Role::TextSubtle:
			return wxColour(100, 100, 100);
		case Role::TextOnAccent:
			return *wxWHITE;
		case Role::Border:
			return wxColour(200, 200, 200);
		case Role::Selected:
			return wxColour(0, 120, 215, 60);
		case Role::Error:
			return wxColour(200, 0, 0);
		case Role::CardBase:
			return *wxWHITE;
		case Role::CardBaseHover:
			return wxColour(245, 245, 245);
		case Role::CardBorder:
			return wxColour(200, 200, 200);
		default:
			return *wxBLACK;
	}
}
