#include "check_control_flow.h"
#include "visitor.h"
#include "replace_restore.hpp"


class CFChecker : public TraversalNodeVisitor {
public:
    virtual void visit_while(S_While &wh) {
        ReplaceRestore<bool> _(&this->inside_loop, true);
        TraversalNodeVisitor::visit_while(wh);
    }

    virtual void visit_func(E_Func &func) {
        ReplaceRestore<bool> _l(&this->inside_loop, false);
        ReplaceRestore<bool> _f(&this->inside_func, true);
        TraversalNodeVisitor::visit_func(func);
    }

    virtual void visit_return(S_Return &ret) {
        if (!this->inside_func) {
            throw BadReturn(ret);
        }
        TraversalNodeVisitor::visit_return(ret);
    }

    virtual void visit_break(S_Break &brk) {
        if (!this->inside_loop) {
            throw BadBreak(brk);
        }
    }

    virtual void visit_continue(S_Continue &cont) {
        if (!this->inside_loop) {
            throw BadContinue(cont);
        }
    }

private:
    bool inside_func = false;
    bool inside_loop = false;
};


void check_control_flow(S_Block &block) {
    CFChecker().visit_block(block);
}
