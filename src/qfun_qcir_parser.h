/*
 * File:  qesto_qcir_parser.h
 * Author:  mikolas
 * Created on:  Fri Jul 28 14:29:19 CEST 2023
 * Copyright (C) 2023, Mikolas Janota
 */
#pragma once
#include "aig.h"
#include "aig_util.h"
#include "auxiliary.h"
#include "qcir_parser.h"
#include "qtypes.h"
#include <unordered_map>
class QfunQCIRParser : public QCIRParser {
  public:
    QfunQCIRParser(StreamBuffer &buf, AigFactory &factory)
        : QCIRParser(buf), d_factory(factory), d_util(factory) {}
    virtual ~QfunQCIRParser() {}

    QAigFla &formula() { return d_qfla; }

    const std::unordered_map<std::string, int> &name2var() const {
        return d_name2var;
    }
    const std::unordered_map<int, std::string> &var2name() const {
        return d_var2name;
    }

  protected:
    virtual void cb_qblock_quant(QType qt) override { d_qt = qt; }

    virtual void cb_qblock_var(std::string v) override {
        d_qcir_var_stack.push_back(make_varid(v));
    }

    virtual void cb_gate_stmt_gt(GType gt) override { d_gate_type = gt; }

    virtual void cb_gate_stmt_var(const std::string &v) override {
        d_gate_var = v;
    }

    virtual void cb_gate_stmt_lit(const Lit &l) override {
        d_qcir_ID_stack.push_back(get_AigLit(l));
    }

    virtual void cb_output_lit(const Lit &l) override { d_output_lit = l; }

    virtual void cb_gate_closed() override {
        define_gate(d_gate_var, make_gate());
        d_qcir_ID_stack.clear();
    }

    AigLit make_gate() {
        auto &args = d_qcir_ID_stack;
        auto &u = d_util;
        switch (d_gate_type) {
        case GType::AND: return u.and_(args, true);
        case GType::OR: return u.or_(args, true);
        case GType::XOR:
            if (d_qcir_ID_stack.size() != 2)
                semantic_error("currently only supporting binary XOR");
            return u.mk_xor(args[0], args[1]);
        case GType::ITE:
            if (d_qcir_ID_stack.size() != 3)
                semantic_error("ITE must have 3 arguments");
            return u.ite(args[0], args[1], args[2]);
        }
        __PL;
        exit(1);
    }

    virtual void cb_quant_closed() override {
        QuantifierType qt;
        switch (d_qt) {
        case QType::EXIST: qt = EXISTENTIAL; break;
        case QType::FORALL: qt = UNIVERSAL; break;
        }
        d_qfla.pref.push_back(Quantification(qt, VarVector(d_qcir_var_stack)));
        d_qcir_var_stack.clear();
    }

    virtual void cb_file_closed() override {
        const auto &[sign, name] = d_output_lit;
        const auto i = d_name2id.find(name);
        if (i == d_name2id.end())
            semantic_error("output gate not defined '" + name + "'");
        d_qfla.matrix = sign ? d_factory.neg(i->second) : i->second;
    }

  protected:
    AigFactory &d_factory;
    AigUtil d_util;
    QAigFla d_qfla;
    Var var_id = 0;
    Lit d_output_lit;
    std::string d_gate_var;
    GType d_gate_type;
    QType d_qt;
    std::vector<AigLit> d_qcir_ID_stack;
    std::vector<Var> d_qcir_var_stack;
    std::unordered_map<std::string, int> d_name2var;
    std::unordered_map<int, std::string> d_var2name;
    std::unordered_map<std::string, AigLit> d_name2id;

    AigLit get_AigLit(const QCIRParser::Lit &l) {
        const auto &[sign, name] = l;
        const auto i = d_name2id.find(name);

        if (i != d_name2id.end()) {
            const auto &retv = i->second;
            return sign ? d_factory.neg(retv) : retv;
        } else {
            const auto vi = d_name2var.find(name);
            if (vi == d_name2var.end())
                semantic_error("undefined variable or gate '" + name + "'");
            const Var v = vi->second;
            const auto retv = d_factory.mk_var(v, sign);
            return retv;
        }
    }

    int make_varid(const std::string &var_name) {
        const auto retv = ++var_id;
        const auto i = d_name2var.insert({var_name, retv});
        if (!i.second)
            semantic_error("var redefinition '" + var_name + "'");
        d_var2name[retv] = var_name;
        return retv;
    }

    AigLit define_gate(const std::string &s, AigLit def) {
        const auto i = d_name2id.insert({s, def});
        if (!i.second)
            semantic_error("gate redefinition");
        return def;
    }

    void semantic_error(std::string msg);
};
