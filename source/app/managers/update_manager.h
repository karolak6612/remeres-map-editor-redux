#ifndef RME_APP_MANAGERS_UPDATE_MANAGER_H_
#define RME_APP_MANAGERS_UPDATE_MANAGER_H_

#include <memory>

class UpdateChecker;
class wxWindow;

class UpdateManager {
public:
	UpdateManager();
	~UpdateManager();

	void Initialize(wxWindow* parent);
	void Unload();

private:
#ifdef _USE_UPDATER_
	std::unique_ptr<UpdateChecker> m_updater;
#endif
};

#endif
