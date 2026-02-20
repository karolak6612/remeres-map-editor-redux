//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "app/main.h"
#include "app/application.h"

#ifdef _USE_UPDATER_

	#include <wx/url.h>

	#include "util/json.h"

	#include "app/updater.h"
	#include "app/application.h"
	#include <thread>
	#include <memory>

wxDEFINE_EVENT(EVT_UPDATE_CHECK_FINISHED, wxCommandEvent);

UpdateChecker::UpdateChecker() {
	////
}

UpdateChecker::~UpdateChecker() {
	////
}

void UpdateChecker::connect(wxEvtHandler* receiver) {
	wxString address = "http://www.remeresmapeditor.com/update.php";
	address << "?os="
			<<
	#ifdef __WINDOWS__
		"windows";
	#elif __LINUX__
		"linux";
	#else
		"unknown";
	#endif
	address << "&verid=" << __RME_VERSION_ID__;
	#ifdef __EXPERIMENTAL__
	address << "&beta";
	#endif

	// Use unique_ptr for automatic cleanup of wxURL
	auto url = std::unique_ptr<wxURL>(newd wxURL(address));

	// Move url into the thread lambda
	std::thread([receiver, url = std::move(url)]() {
		// Use unique_ptr for wxInputStream. wxURL::GetInputStream returns a new stream that we must delete.
		std::unique_ptr<wxInputStream> input(url->GetInputStream());
		if (!input) {
			return;
		}

		std::string data;
		while (!input->Eof()) {
			data += input->GetC();
		}
		// input and url are automatically deleted here

		// Post event to main thread
		wxGetApp().CallAfter([receiver, data]() {
			if (receiver) {
				wxCommandEvent event(EVT_UPDATE_CHECK_FINISHED);
				// Use SetString to pass data safely without manual memory management
				event.SetString(data);
				receiver->AddPendingEvent(event);
			}
		});
	}).detach();
}

#endif
