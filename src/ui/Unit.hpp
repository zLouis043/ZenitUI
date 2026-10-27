#pragma once

#include "Common.hpp"
#include "CoreTypes.hpp"
#include "SmallVector.hpp"

namespace ZenitUI
{

    enum class Unit
    {
        Auto = -1,
        Number = 0,
        Pixel = 1,
        Percent = 2,
        VW = 3,
        VH = 4,
        PW = 5,
        PH = 6
    };

    enum class ValueOp
    {
        None,
        Add,
        Sub,
        Mul,
        Div,
        Min,
        Max,
        Clamp,
        Round,
        Floor,
        Ceil,
        Mod
    };

    struct Value
    {
        struct Term
        {
            float coeff{0.0f};
            Unit unit{Unit::Auto};

            bool operator==(const Term &o) const
            {
                return coeff == o.coeff && unit == o.unit;
            }
        };

        SmallVector<Term, 2> terms;
        ValueOp op{ValueOp::None};
        std::shared_ptr<std::vector<Value>> args;

        Value() = default;
        Value(float pixels) : terms{{pixels, Unit::Pixel}} {}
        Value(float amount, Unit u)
        {
            if (u != Unit::Auto)
                terms.push_back({amount, u});
        }

        // ---------- Factory ----------
        static Value autoSize() { return {}; }
        static Value number(float n)
        {
            Value v;
            v.terms = {{n, Unit::Number}};
            return v;
        }
        static Value px(float p)
        {
            Value v;
            v.terms = {{p, Unit::Pixel}};
            return v;
        }
        static Value percent(float p)
        {
            Value v;
            v.terms = {{p, Unit::Percent}};
            return v;
        }
        static Value vw(float p)
        {
            Value v;
            v.terms = {{p, Unit::VW}};
            return v;
        }
        static Value vh(float p)
        {
            Value v;
            v.terms = {{p, Unit::VH}};
            return v;
        }
        static Value pw(float p)
        {
            Value v;
            v.terms = {{p, Unit::PW}};
            return v;
        }
        static Value ph(float p)
        {
            Value v;
            v.terms = {{p, Unit::PH}};
            return v;
        }

        bool isAuto() const { return terms.empty(); }
        bool isSimple() const { return terms.size() == 1; }
        bool isNumber() const { return terms.size() == 1 && terms[0].unit == Unit::Number; }
        float asNumber() const { return isNumber() ? terms[0].coeff : 0.0f; }

    private:
        float resolveArg(size_t idx, float pw, float ph, float autoV, int mode) const
        {
            if (!args || idx >= args->size())
                return 0.0f;
            const auto &a = args->at(idx);
            if (mode == 0)
                return a.resolveH(pw, ph, autoV);
            if (mode == 1)
                return a.resolveV(pw, ph, autoV);
            if (mode == 2)
                return a.resolveSelfH(pw, ph);
            return a.resolveSelfV(pw, ph);
        }

        float resolveAST(float pw, float ph, float autoV, int mode) const
        {
            if (!args)
                return 0.0f;
            switch (op)
            {
            case ValueOp::Add:
                return resolveArg(0, pw, ph, autoV, mode) + resolveArg(1, pw, ph, autoV, mode);
            case ValueOp::Sub:
                return resolveArg(0, pw, ph, autoV, mode) - resolveArg(1, pw, ph, autoV, mode);
            case ValueOp::Mul:
                return resolveArg(0, pw, ph, autoV, mode) * resolveArg(1, pw, ph, autoV, mode);
            case ValueOp::Div:
            {
                float b = resolveArg(1, pw, ph, autoV, mode);
                return b != 0.0f ? resolveArg(0, pw, ph, autoV, mode) / b : 0.0f;
            }
            case ValueOp::Min:
            {
                if (args->empty())
                    return 0.0f;
                float r = resolveArg(0, pw, ph, autoV, mode);
                for (size_t i = 1; i < args->size(); ++i)
                    r = std::min(r, resolveArg(i, pw, ph, autoV, mode));
                return r;
            }
            case ValueOp::Max:
            {
                if (args->empty())
                    return 0.0f;
                float r = resolveArg(0, pw, ph, autoV, mode);
                for (size_t i = 1; i < args->size(); ++i)
                    r = std::max(r, resolveArg(i, pw, ph, autoV, mode));
                return r;
            }
            case ValueOp::Clamp:
            {
                if (args->size() >= 3)
                {
                    float minV = resolveArg(0, pw, ph, autoV, mode);
                    float valV = resolveArg(1, pw, ph, autoV, mode);
                    float maxV = resolveArg(2, pw, ph, autoV, mode);
                    return std::clamp(valV, minV, maxV);
                }
                return 0.0f;
            }
            case ValueOp::Round:
                return args->empty() ? 0.0f : std::round(resolveArg(0, pw, ph, autoV, mode));
            case ValueOp::Floor:
                return args->empty() ? 0.0f : std::floor(resolveArg(0, pw, ph, autoV, mode));
            case ValueOp::Ceil:
                return args->empty() ? 0.0f : std::ceil(resolveArg(0, pw, ph, autoV, mode));
            case ValueOp::Mod:
            {
                if (args->size() >= 2)
                {
                    float a = resolveArg(0, pw, ph, autoV, mode);
                    float b = resolveArg(1, pw, ph, autoV, mode);
                    return b != 0.0f ? std::fmod(a, b) : 0.0f;
                }
                return 0.0f;
            }
            default:
                return 0.0f;
            }
        }
public:
        // ---------- Risoluzione (4 varianti) ----------
        float resolveH(float parentW, float parentH, float autoValue = 0.0f) const
        {
            if (op != ValueOp::None)
                return resolveAST(parentW, parentH, autoValue, 0);
            if (terms.empty())
                return autoValue;
            float result = 0.0f;
            for (const auto &t : terms)
            {
                switch (t.unit)
                {
                case Unit::Number:
                    result += t.coeff;
                    break;
                case Unit::Pixel:
                    result += t.coeff;
                    break;
                case Unit::Percent:
                    result += t.coeff * 0.01f * parentW;
                    break;
                case Unit::PW:
                    result += t.coeff * 0.01f * parentW;
                    break;
                case Unit::PH:
                    result += t.coeff * 0.01f * parentH;
                    break;
                case Unit::VW:
                    result += t.coeff * 0.01f * Metrics::viewport.x;
                    break;
                case Unit::VH:
                    result += t.coeff * 0.01f * Metrics::viewport.y;
                    break;
                default:
                    break;
                }
            }
            return result;
        }

        float resolveV(float parentW, float parentH, float autoValue = 0.0f) const
        {
            if (op != ValueOp::None)
                return resolveAST(parentW, parentH, autoValue, 0);
            if (terms.empty())
                return autoValue;
            float result = 0.0f;
            for (const auto &t : terms)
            {
                switch (t.unit)
                {
                case Unit::Number:
                    result += t.coeff;
                    break;
                case Unit::Pixel:
                    result += t.coeff;
                    break;
                case Unit::Percent:
                    result += t.coeff * 0.01f * parentH;
                    break;
                case Unit::PW:
                    result += t.coeff * 0.01f * parentW;
                    break;
                case Unit::PH:
                    result += t.coeff * 0.01f * parentH;
                    break;
                case Unit::VW:
                    result += t.coeff * 0.01f * Metrics::viewport.x;
                    break;
                case Unit::VH:
                    result += t.coeff * 0.01f * Metrics::viewport.y;
                    break;
                default:
                    break;
                }
            }
            return result;
        }

        float resolveSelfH(float selfW, float selfH) const
        {
            if (op != ValueOp::None)
                return resolveAST(selfW, selfH, 0.0f, 2);
            if (terms.empty())
                return 0.0f;
            float result = 0.0f;
            for (const auto &t : terms)
            {
                switch (t.unit)
                {
                case Unit::Number:
                    result += t.coeff;
                    break;
                case Unit::Pixel:
                    result += t.coeff;
                    break;
                case Unit::Percent:
                    result += t.coeff * 0.01f * selfW;
                    break;
                case Unit::PW:
                    result += t.coeff * 0.01f * selfW;
                    break;
                case Unit::PH:
                    result += t.coeff * 0.01f * selfH;
                    break;
                case Unit::VW:
                    result += t.coeff * 0.01f * Metrics::viewport.x;
                    break;
                case Unit::VH:
                    result += t.coeff * 0.01f * Metrics::viewport.y;
                    break;
                default:
                    break;
                }
            }
            return result;
        }

        float resolveSelfV(float selfW, float selfH) const
        {
            if (op != ValueOp::None)
                return resolveAST(selfW, selfH, 0.0f, 3);
            if (terms.empty())
                return 0.0f;
            float result = 0.0f;
            for (const auto &t : terms)
            {
                switch (t.unit)
                {
                case Unit::Number:
                    result += t.coeff;
                    break;
                case Unit::Pixel:
                    result += t.coeff;
                    break;
                case Unit::Percent:
                    result += t.coeff * 0.01f * selfH;
                    break;
                case Unit::PW:
                    result += t.coeff * 0.01f * selfW;
                    break;
                case Unit::PH:
                    result += t.coeff * 0.01f * selfH;
                    break;
                case Unit::VW:
                    result += t.coeff * 0.01f * Metrics::viewport.x;
                    break;
                case Unit::VH:
                    result += t.coeff * 0.01f * Metrics::viewport.y;
                    break;
                default:
                    break;
                }
            }
            return result;
        }

        // ---------- Operazioni ----------
        Value operator+(const Value &o) const
        {
            if (op != ValueOp::None || o.op != ValueOp::None)
            {
                Value r;
                r.op = ValueOp::Add;
                r.args = std::make_shared<std::vector<Value>>();
                r.args->push_back(*this);
                r.args->push_back(o);
                return r;
            }
            Value r;
            r.terms.reserve(terms.size() + o.terms.size());
            for (auto &t : terms)
                r.terms.push_back(t);
            for (auto &t : o.terms)
                r.terms.push_back(t);
            r.normalize();
            return r;
        }

        Value operator-(const Value &o) const
        {
            if (op != ValueOp::None || o.op != ValueOp::None)
            {
                Value r;
                r.op = ValueOp::Sub;
                r.args = std::make_shared<std::vector<Value>>();
                r.args->push_back(*this);
                r.args->push_back(o);
                return r;
            }
            Value r;
            r.terms.reserve(terms.size() + o.terms.size());
            for (auto &t : terms)
                r.terms.push_back(t);
            for (auto &t : o.terms)
                r.terms.push_back({-t.coeff, t.unit});
            r.normalize();
            return r;
        }

        Value operator-() const
        {
            if (op != ValueOp::None)
            {
                Value r;
                r.op = ValueOp::Mul;
                r.args = std::make_shared<std::vector<Value>>();
                r.args->push_back(*this);
                r.args->push_back(Value::number(-1.0f));
                return r;
            }
            Value r;
            r.terms.reserve(terms.size());
            for (auto &t : terms)
                r.terms.push_back({-t.coeff, t.unit});
            return r;
        }

        Value operator*(float k) const
        {
            if (op != ValueOp::None)
            {
                Value r;
                r.op = ValueOp::Mul;
                r.args = std::make_shared<std::vector<Value>>();
                r.args->push_back(*this);
                r.args->push_back(Value::number(k));
                return r;
            }
            Value r;
            r.terms.reserve(terms.size());
            for (auto &t : terms)
                r.terms.push_back({t.coeff * k, t.unit});
            return r;
        }

        Value operator/(float k) const
        {
            if (k == 0.0f) return *this;
            if (op != ValueOp::None)
            {
                Value r;
                r.op = ValueOp::Div;
                r.args = std::make_shared<std::vector<Value>>();
                r.args->push_back(*this);
                r.args->push_back(Value::number(k));
                return r;
            }
            return (*this) * (1.0f / k);
        }

        Value mulWith(const Value &o) const
        {
            if (op != ValueOp::None || o.op != ValueOp::None)
            {
                Value r;
                r.op = ValueOp::Mul;
                r.args = std::make_shared<std::vector<Value>>();
                r.args->push_back(*this);
                r.args->push_back(o);
                return r;
            }
            if (isNumber())
                return o * terms[0].coeff;
            if (o.isNumber())
                return *this * o.terms[0].coeff;
            return *this;
        }

        Value divWith(const Value &o) const
        {
            if (op != ValueOp::None || o.op != ValueOp::None)
            {
                Value r;
                r.op = ValueOp::Div;
                r.args = std::make_shared<std::vector<Value>>();
                r.args->push_back(*this);
                r.args->push_back(o);
                return r;
            }
            if (o.isNumber() && o.terms[0].coeff != 0.0f)
                return *this * (1.0f / o.terms[0].coeff);
            return *this;
        }

        bool operator==(const Value &o) const
        {
            if (terms.size() != o.terms.size())
                return false;
            for (size_t i = 0; i < terms.size(); ++i)
                if (!(terms[i] == o.terms[i]))
                    return false;
            return true;
        }
        bool operator!=(const Value &o) const { return !(*this == o); }

        void normalize()
        {
            float coeffs[7] = {0};
            bool present[7] = {false};
            for (const auto &t : terms)
            {
                int idx = static_cast<int>(t.unit);
                if (idx < 0 || idx >= 7)
                    continue;
                coeffs[idx] += t.coeff;
                present[idx] = true;
            }
            terms.clear();
            for (int i = 0; i < 7; ++i)
            {
                if (present[i] && coeffs[i] != 0.0f)
                    terms.push_back({coeffs[i], static_cast<Unit>(i)});
            }
        }
    };

    inline Value Auto() { return Value::autoSize(); }
    inline Value Px(float p) { return Value::px(p); }
    inline Value Percent(float p) { return Value::percent(p); }
    inline Value VW(float p) { return Value::vw(p); }
    inline Value VH(float p) { return Value::vh(p); }
    inline Value PW(float p) { return Value::pw(p); }
    inline Value PH(float p) { return Value::ph(p); }
    inline Value Num(float p) { return Value::number(p); }

} // namespace ZenitUI