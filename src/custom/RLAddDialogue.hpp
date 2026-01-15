#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLAddDialogue : public geode::Popup<> {
     public:
      static RLAddDialogue* create();

     private:
      bool setup() override;
      TextInput* m_dialogueInput = nullptr;
      utils::web::WebTask m_setDialogueTask;
      ~RLAddDialogue() { m_setDialogueTask.cancel(); }
      void onSubmit(CCObject* sender);
      void onPreview(CCObject* sender);
};