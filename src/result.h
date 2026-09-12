// A tiny Result<T> for railway-oriented error handling: functions that can
// fail return a Result instead of throwing, and callers pattern-match on
// ok()/error() instead of wrapping calls in try/catch. Mirrors the shape
// the MediaPipe bridge already uses (Create() returns null + fills
// *error), just made explicit and reusable on the app side.
#pragma once

#include <optional>
#include <string>
#include <utility>

namespace miaucam {

template <typename T>
class Result {
public:
    static Result Ok(T value) { return Result(std::move(value), {}); }
    static Result Err(std::string error) { return Result(std::nullopt, std::move(error)); }

    bool ok() const { return value_.has_value(); }
    explicit operator bool() const { return ok(); }

    const T& value() const { return *value_; }
    T& value() { return *value_; }
    const std::string& error() const { return error_; }

private:
    Result(std::optional<T> value, std::string error)
        : value_(std::move(value)), error_(std::move(error)) {}

    std::optional<T> value_;
    std::string error_;
};

}  // namespace miaucam
