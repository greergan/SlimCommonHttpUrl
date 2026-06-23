case ParseState::HOST: {
    // one-time lookahead for '@' when we first enter HOST
    if (host_coords.first == static_cast<int>(string_position)) {
        for (size_t i = string_position; i < string_length; ++i) {
            const char c = s[i];
            if (c == '/' || c == '?' || c == '#') break;
            if (c == '@') {
                username_coords.first = static_cast<int>(string_position);
                for (size_t j = string_position; j < i; ++j) {
                    if (s[j] == ':') {
                        username_coords.second = static_cast<int>(j) - 1;
                        password_coords.first  = static_cast<int>(j) + 1;
                        password_coords.second = static_cast<int>(i) - 1;
                        break;
                    }
                }
                if (password_coords.first == -1) {
                    username_coords.second = static_cast<int>(i) - 1;
                }
                host_coords.first = static_cast<int>(i) + 1;
                string_position = static_cast<size_t>(host_coords.first);
                break;
            }
        }
        // guard: '@' with nothing after it
        if (string_position >= string_length) {
            error = ErrorStatus::UrlHostMissing;
            state = ParseState::INVALID;
            break;
        }
    }
    // re-read character after possible lookahead advance
    const unsigned char current = static_cast<unsigned char>(s[string_position]);
    if (host_coords.first == static_cast<int>(string_position) && !slim::common::utilities::is_alnum(static_cast<char>(current))) {
        error = ErrorStatus::UrlHostInvalidStart;
        state = ParseState::INVALID;
    }
    else if (current == '/') {
    // ... rest of HOST state uses `current` not `character`
