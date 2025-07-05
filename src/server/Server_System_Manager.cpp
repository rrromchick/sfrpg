#include "Server_System_Manager.h"

ServerSystemManager::ServerSystemManager() {
    addSystem<S_Network>(System::Network);
    addSystem<S_Movement>(System::Movement);
    addSystem<S_Control>(System::Control);
    addSystem<S_State>(System::State);
    addSystem<S_Collision>(System::Collision);
    addSystem<S_Combat>(System::Combat);
    addSystem<S_Timers>(System::Timers);
}

ServerSystemManager::~ServerSystemManager() {}