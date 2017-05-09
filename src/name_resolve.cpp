#include <functional>
#include <vector>

#include "name_resolve.h"


static void iter_blocks(Node &stmt, std::function<void (S_Block &)> callback) {
    if (S_Block *block = dynamic_cast<S_Block *>(&stmt)) {
        callback(*block);
    } else if (S_While *wh = dynamic_cast<S_While *>(&stmt)) {
        callback(static_cast<S_Block &>(*wh->block));
    } else if (S_Condition *cond = dynamic_cast<S_Condition *>(&stmt)) {
        callback(static_cast<S_Block &>(*cond->then_block));
        if (cond->else_block) {
            iter_blocks(*cond->else_block, callback);
        }
    }
    // TODO: for, do-while
}


static void iter_terminal_exp_shallow(Node &node, std::function<void(Node &)> callback) {
    Node *pnode = &node;
    if (S_DeclareList *decls = dynamic_cast<S_DeclareList *>(pnode)) {
        for (const auto &pair : decls->decls) {
            if (pair.initial) {
                iter_terminal_exp_shallow(*pair.initial, callback);
            }
        }
    } else if (S_Condition *cond = dynamic_cast<S_Condition *>(pnode)) {
        iter_terminal_exp_shallow(*cond->condition, callback);
        if (cond->else_block) {
            iter_terminal_exp_shallow(*cond->else_block, callback);
        }
    } else if (S_While *wh = dynamic_cast<S_While *>(pnode)) {
        iter_terminal_exp_shallow(*wh->condition, callback);
    // TODO: for do-while
    } else if (S_Return *ret = dynamic_cast<S_Return *>(pnode)) {
        if (ret->value) {
            iter_terminal_exp_shallow(*ret->value, callback);
        }
    } else if (dynamic_cast<S_Block *>(pnode) || dynamic_cast<S_Continue *>(pnode)
        || dynamic_cast<S_Break *>(pnode) || dynamic_cast<S_Empty *>(pnode))
    {
        // pass
    } else if (S_Exp *stmt = dynamic_cast<S_Exp *>(pnode)) {
        iter_terminal_exp_shallow(*stmt->value, callback);
    } else if (E_Op *exp = dynamic_cast<E_Op *>(pnode)) {
        callback(*exp);
        for (Node::Ptr &arg : exp->args) {
            iter_terminal_exp_shallow(*arg, callback);
        }
    } else if (E_List *list = dynamic_cast<E_List *>(pnode)) {
        // callback(*list);
        for (Node::Ptr &item : list->value) {
            iter_terminal_exp_shallow(*item, callback);
        }
    } else {
        // terminal expression
        callback(*pnode);
    }
}


static void iter_funcs(Node &stmt, std::function<void (E_Func &)> callback) {
    iter_terminal_exp_shallow(stmt, [&](Node &node) {
        if (E_Func *func = dynamic_cast<E_Func *>(&node)) {
            callback(*func);
        }
    });
}


static void iter_vars(Node &stmt, std::function<void (E_Var &)> callback) {
    iter_terminal_exp_shallow(stmt, [&](Node &node) {
        if (E_Var *var = dynamic_cast<E_Var *>(&node)) {
            callback(*var);
        }
    });
}


static void add_declarations_to_block_attr(S_Block::AttrType &attr, const S_DeclareList &decls) {
    for (const auto &pair : decls.decls) {
        auto it = attr.name_to_local_index.find(pair.name);
        if (it != attr.name_to_local_index.end()) {
            throw DuplicatedLocalName("Duplicated local name: " + u8_encode(pair.name));
        }

        attr.name_to_local_index.emplace(pair.name, attr.local_info.size());
        attr.local_info.emplace_back(pair.name);
    }
}


S_Block::AttrType::NonLocalInfo resolve_from_block(S_Block *block, const ustring &name) {
    for (; block != nullptr; block = block->attr.parent) {
        auto it = block->attr.name_to_local_index.find(name);
        if (it != block->attr.name_to_local_index.end()) {
            return {block, it->second};
        }
    }

    throw NoSuchName("No such name: " + u8_encode(name));
}


static int add_nonlocal_to_block_attr(S_Block::AttrType &attr, const ustring &name, S_Block *start)
{
    auto it = attr.name_to_nonlocal_index.find(name);
    if (it == attr.name_to_nonlocal_index.end()) {
        int index = static_cast<int>(attr.nonlocal_indexes.size());
        attr.name_to_nonlocal_index.emplace(name, index);
        attr.nonlocal_indexes.emplace_back(resolve_from_block(start, name));
        return index;
    } else {
        return it->second;
    }
}


void resolve_names(S_Block &block) {
    for (Node::Ptr &stmt : block.stmts) {
        S_DeclareList *decls = dynamic_cast<S_DeclareList *>(stmt.get());
        if (decls != nullptr) {
            add_declarations_to_block_attr(block.attr, *decls);
        }
    }

    for (Node::Ptr &stmt : block.stmts) {
        // TODO: for stmt
        iter_blocks(*stmt, [&](S_Block &child_block) {
            child_block.attr.parent = &block;
            resolve_names(child_block);
        });

        iter_vars(*stmt, [&](E_Var &var) {
            auto it = block.attr.name_to_local_index.find(var.name);
            if (it != block.attr.name_to_local_index.end()) {
                var.attr.is_local = true;
                var.attr.index = it->second;
            } else {
                var.attr.is_local = false;
                var.attr.index = add_nonlocal_to_block_attr(
                    block.attr, var.name, block.attr.parent
                );
            }
        });

        iter_funcs(*stmt, [&](E_Func &func) {
            S_Block &func_block = static_cast<S_Block &>(*func.block);
            if (func.args) {
                iter_vars(*func.args, [&](E_Var &var) {
                    var.attr.is_local = false;
                    var.attr.index = add_nonlocal_to_block_attr(
                        func_block.attr, var.name, &block
                    );
                });

                add_declarations_to_block_attr(
                    func_block.attr, static_cast<S_DeclareList &>(*func.args)
                );
            }

            func_block.attr.parent = &block;
            resolve_names(func_block);
        });
    }
}
