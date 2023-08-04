#include "readq.h"
#include "read_exception.h"
#include "reader.h"
#include <algorithm>
#include <cmath> // for abs
#include <cstdio>
#include <sstream>
#include <string> // for string
using SATSPC::mkLit;
using std::max;
using std::string;

ReadQ::ReadQ(Reader &r, bool _qube_input)
    : r(r), qube_input(_qube_input), qube_output(-1), max_id(0),
      _header_read(false) {}

ReadQ::~ReadQ() {}

bool ReadQ::get_header_read() const { return _header_read; }

Var ReadQ::get_max_id() const { return max_id; }

const vector<Quantification> &ReadQ::get_prefix() const {
    return quantifications;
}
const vector<LitSet> &ReadQ::get_clauses() const { return clause_vector; }
int ReadQ::get_qube_output() const {
    assert(qube_input);
    return qube_output;
}

void ReadQ::read_cnf_clause(Reader &in, vector<Lit> &lits) {
    int parsed_lit;
    lits.clear();
    for (;;) {
        in.skip_whitespace();
        parsed_lit = parse_lit(in);
        if (parsed_lit == 0)
            break;
        const Var v = abs(parsed_lit);
        max_id = max(max_id, v);
        if (!contains(quantified_variables, v))
            unquantified_variables.insert(v);
        lits.push_back(parsed_lit > 0 ? mkLit(v) : ~mkLit(v));
    }
}

void ReadQ::read_quantification(Reader &in, Quantification &quantification) {
    const char qchar = *in;
    ++in;
    if (qchar == 'a')
        quantification.first = UNIVERSAL;
    else if (qchar == 'e')
        quantification.first = EXISTENTIAL;
    else
        throw ReadException("unexpeceted quantifier");

    // vector<Var> variables;
    auto &variables = quantification.second;
    while (true) {
        in.skip_whitespace();
        if (*in == EOF)
            throw ReadException("quantifier not terminated by 0");
        const Var v = parse_variable(in);
        if (v == 0) {
            do {
                in.skip_line();
            } while (*in == 'c');
            if (*in != qchar)
                break;
            ++in;
        } else {
            max_id = max(max_id, v);
            variables.push_back(v);
            quantified_variables.insert(v);
        }
    }
    quantification.second = VarVector(variables);
}

Var ReadQ::parse_variable(Reader &in) {
    if (*in < '0' || *in > '9') {
        std::stringstream ss;
        ss << in.get_line_number() << ":"
           << "unexpected char in place of a variable: '"
           << static_cast<char>(*in) << "'";
        throw ReadException(ss.str());
    }
    Var return_value = 0;
    while (*in >= '0' && *in <= '9') {
        return_value = return_value * 10 + (*in - '0');
        ++in;
    }
    return return_value;
}

int ReadQ::parse_lit(Reader &in) {
    Var return_value = 0;
    bool neg = false;
    if (*in == '-') {
        neg = true;
        ++in;
    } else if (*in == '+')
        ++in;
    if ((*in < '0') || (*in > '9')) {
        std::stringstream ss;
        ss << in.get_line_number() << ":"
           << "unexpected char in place of a variable: '"
           << static_cast<char>(*in) << "'";
        throw ReadException(ss.str());
    }
    while (*in >= '0' && *in <= '9') {
        return_value = return_value * 10 + (*in - '0');
        ++in;
    }
    if (neg)
        return_value = -return_value;
    return return_value;
}

void ReadQ::read_header() {
    while (*r == 'c')
        r.skip_line();
    if (*r == 'p') {
        _header_read = true;
        r.skip_line();
    }
}

void ReadQ::read_quantifiers() {
    for (;;) {
        if (*r == 'c') {
            r.skip_line();
            continue;
        }
        if (*r != 'e' && *r != 'a')
            break;
        const auto sz = quantifications.size();
        quantifications.resize(sz + 1);
        read_quantification(r, quantifications[sz]);
    }
}

void ReadQ::read_clauses() {
    Reader &in = r;
    vector<Lit> ls;
    for (;;) {
        in.skip_whitespace();
        if (*in == EOF)
            break;
        if (*r == 'c') {
            in.skip_line();
            continue;
        }
        ls.clear();
        read_cnf_clause(in, ls);
        clause_vector.push_back(LitSet::mk(ls));
    }
}

void ReadQ::read_word(const char *word, size_t length) {
    while (length) {
        if (word[0] != *r) {
            string s("unexpected char in place of: ");
            s += word[0];
            throw ReadException(s);
        }
        ++r;
        --length;
        ++word;
    }
}

void ReadQ::read() {
    read_header();

    if (qube_input && (*r == 's')) {
        ++r;
        r.skip_whitespace();
        read_word("cnf", 3);
        r.skip_whitespace();
        std::cerr << "code: " << *r << std::endl;
        if (*r == '0')
            qube_output = 0;
        else if (*r == '1')
            qube_output = 1;
        else {
            string s("unexpected char in place of 0/1");
            throw ReadException(s);
        }
        return;
    }

    read_quantifiers();
    read_clauses();

    if (!unquantified_variables.empty()) {
        if (!quantifications.empty() &&
            quantifications[0].first == EXISTENTIAL) {
            vector<Var> variables;
            for (auto v : quantifications[0].second)
                variables.push_back(v);
            for (auto v : unquantified_variables)
                variables.push_back(v);
            quantifications[0].second = VarVector(variables);
        } else {
            Quantification quantification;
            quantification.first = EXISTENTIAL;
            // vector<Var> variables;
            // FOR_EACH(vi,unquantified_variables) variables.push_back(*vi);
            // quantification.second=VarVector(variables);
            for (Var v : unquantified_variables)
                quantification.second.push_back(v);
            quantifications.insert(quantifications.begin(), quantification);
        }
    }
}
