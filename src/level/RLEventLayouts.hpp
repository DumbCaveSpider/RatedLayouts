#include <Geode/Geode.hpp>

using namespace geode::prelude;

class RLEventLayouts : public geode::Popup<> {
     public:
      static RLEventLayouts* create();

     private:
      bool setup() override;
};