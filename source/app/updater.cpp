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
	wxURL* url = newd wxURL(address);

	m_thread = std::jthread([receiver, url]() {
		wxInputStream* input = url->GetInputStream();
		if (!input) {
			delete input; // Safe to delete null? Yes.
			delete url;
			return;
		}

		std::string data;
		while (!input->Eof()) {
			data += input->GetC();
		}

		delete input;
		delete url;

		// We use raw pointer 'receiver' because Application manages lifetime of UpdateChecker
		// and ensures thread is joined before receiver (MainFrame) is destroyed.
		wxGetApp().CallAfter([receiver, data]() {
			if (receiver) {
				wxCommandEvent event(EVT_UPDATE_CHECK_FINISHED);
				event.SetClientData(newd std::string(data));
				receiver->AddPendingEvent(event);
			}
		});
	});
}

#endif
