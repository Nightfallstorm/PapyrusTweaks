#pragma once

namespace Util
{
	namespace stl
	{
		namespace string
		{
			namespace detail
			{
				// trim from left
				inline std::string& ltrim(std::string& a_str)
				{
					a_str.erase(0, a_str.find_first_not_of(" \t\n\r\f\v"));
					return a_str;
				}

				// trim from right
				inline std::string& rtrim(std::string& a_str)
				{
					a_str.erase(a_str.find_last_not_of(" \t\n\r\f\v") + 1);
					return a_str;
				}
			}

			inline std::string& trim(std::string& a_str)
			{
				return detail::ltrim(detail::rtrim(a_str));
			}

			inline std::string trim_copy(std::string a_str)
			{
				return trim(a_str);
			}

			inline bool is_empty(const char* a_char)
			{
				return a_char == nullptr || a_char[0] == '\0';
			}

			inline bool is_only_digit(std::string_view a_str)
			{
				return std::ranges::all_of(a_str, [](char c) {
					return std::isdigit(static_cast<unsigned char>(c));
				});
			}

			inline bool is_only_hex(std::string_view a_str)
			{
				if (a_str.compare(0, 2, "0x") == 0 || a_str.compare(0, 2, "0X") == 0) {
					return a_str.size() > 2 && std::all_of(a_str.begin() + 2, a_str.end(), [](char c) {
						return std::isxdigit(static_cast<unsigned char>(c));
					});
				}
				return false;
			}

			inline bool is_only_letter(std::string_view a_str)
			{
				return std::ranges::all_of(a_str, [](char c) {
					return std::isalpha(static_cast<unsigned char>(c));
				});
			}

			inline bool is_only_space(std::string_view a_str)
			{
				return std::ranges::all_of(a_str, [](char c) {
					return std::isspace(static_cast<unsigned char>(c));
				});
			}

			inline bool icontains(std::string_view a_str1, std::string_view a_str2)
			{
				if (a_str2.length() > a_str1.length())
					return false;

				auto found = std::ranges::search(a_str1, a_str2,
					[](char ch1, char ch2) {
						return std::toupper(static_cast<unsigned char>(ch1)) == std::toupper(static_cast<unsigned char>(ch2));
					});

				return !found.empty();
			}

			inline bool iequals(std::string_view a_str1, std::string_view a_str2)
			{
				return std::ranges::equal(a_str1, a_str2,
					[](char ch1, char ch2) {
						return std::toupper(static_cast<unsigned char>(ch1)) == std::toupper(static_cast<unsigned char>(ch2));
					});
			}

			inline bool istartsWith(std::string_view a_string, std::string_view a_subString)
			{
				if (a_subString.length() > a_string.length()) {
					return false;
				}
				return iequals(a_string.substr(0, a_subString.length()), a_subString);
			}

			inline std::string join(const std::vector<std::string>& a_vec, const char* a_delimiter)
			{
				std::ostringstream os;
				auto               begin = a_vec.begin();
				auto               end = a_vec.end();

				if (begin != end) {
					std::copy(begin, std::prev(end), std::ostream_iterator<std::string>(os, a_delimiter));
					os << *std::prev(end);
				}

				return os.str();
			}

			template <class T>
			T lexical_cast(const std::string& a_str, bool a_hex = false)
			{
				auto base = a_hex ? 16 : 10;

				if constexpr (std::is_floating_point_v<T>) {
					return static_cast<T>(std::stof(a_str));
				} else if constexpr (std::is_signed_v<T>) {
					return static_cast<T>(std::stoi(a_str, nullptr, base));
				} else if constexpr (sizeof(T) == sizeof(std::uint64_t)) {
					return static_cast<T>(std::stoull(a_str, nullptr, base));
				} else {
					return static_cast<T>(std::stoul(a_str, nullptr, base));
				}
			}

			inline std::string remove_non_alphanumeric(std::string& a_str)
			{
				std::ranges::replace_if(
					a_str, [](char c) { return !std::isalnum(static_cast<unsigned char>(c)); }, ' ');
				return trim_copy(a_str);
			}

			inline std::string remove_non_numeric(std::string& a_str)
			{
				std::ranges::replace_if(
					a_str, [](char c) { return !std::isdigit(static_cast<unsigned char>(c)); }, ' ');
				return trim_copy(a_str);
			}

			inline void replace_all(std::string& a_str, std::string_view a_search, std::string_view a_replace)
			{
				if (a_search.empty()) {
					return;
				}

				size_t pos = 0;
				while ((pos = a_str.find(a_search, pos)) != std::string::npos) {
					a_str.replace(pos, a_search.length(), a_replace);
					pos += a_replace.length();
				}
			}

			inline void replace_first_instance(std::string& a_str, std::string_view a_search, std::string_view a_replace)
			{
				if (a_search.empty()) {
					return;
				}

				if (auto pos = a_str.find(a_search); pos != std::string::npos) {
					a_str.replace(pos, a_search.length(), a_replace);
				}
			}

			inline void replace_last_instance(std::string& a_str, std::string_view a_search, std::string_view a_replace)
			{
				if (a_search.empty()) {
					return;
				}

				if (auto pos = a_str.rfind(a_search); pos != std::string::npos) {
					a_str.replace(pos, a_search.length(), a_replace);
				}
			}

			inline std::vector<std::string> split(const std::string& a_str, const std::string& a_deliminator)
			{
				std::vector<std::string> list;
				std::string              strCopy = a_str;
				size_t                   pos = 0;
				std::string              token;
				while ((pos = strCopy.find(a_deliminator)) != std::string::npos) {
					token = strCopy.substr(0, pos);
					list.push_back(token);
					strCopy.erase(0, pos + a_deliminator.length());
				}
				list.push_back(strCopy);
				return list;
			}
		}

		// https://stackoverflow.com/questions/5056645/sorting-stdmap-using-value
		template <typename A, typename B>
		std::multimap<B, A> flip_map(std::map<A, B>& src)
		{
			std::multimap<B, A> dst;

			for (typename std::map<A, B>::const_iterator it = src.begin(); it != src.end(); ++it)
				dst.insert(std::pair<B, A>(it->second, it->first));

			return dst;
		}
		template <typename First, typename... T>
		[[nodiscard]] bool is_in(First&& first, T&&... t)
		{
			return ((first == t) || ...);
		}
	}
}
