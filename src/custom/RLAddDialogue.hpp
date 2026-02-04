#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;

class RLAddDialogue : public geode::Popup {
     public:
      static RLAddDialogue* create();

     private:
      bool init();
      TextInput* m_dialogueInput = nullptr;
      async::TaskHolder<web::WebResponse> m_setDialogueTask;
      ~RLAddDialogue() { m_setDialogueTask.cancel(); }
      void onSubmit(CCObject* sender);
      void onPreview(CCObject* sender);
};