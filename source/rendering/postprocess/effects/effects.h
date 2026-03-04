#ifndef RME_RENDERING_POSTPROCESS_EFFECTS_H_
#define RME_RENDERING_POSTPROCESS_EFFECTS_H_

class PostProcessManager;

void RegisterScanlineEffect(PostProcessManager &manager);
void RegisterXBRZEffect(PostProcessManager &manager);
void RegisterScreenEffect(PostProcessManager &manager);

#endif
