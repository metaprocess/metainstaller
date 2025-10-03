#ifndef MARKDOWNTOHTML_H
#define MARKDOWNTOHTML_H

#include <string>
#include <vector>
#include <regex>

class MarkdownToHtml {
public:
    static std::string convert(const std::string& markdown);
    
private:
    static std::string processHeaders(const std::string& text);
    static std::string processCodeBlocks(const std::string& text);
    static std::string processInlineCode(const std::string& text);
    static std::string processBold(const std::string& text);
    static std::string processItalic(const std::string& text);
    static std::string processLinks(const std::string& text);
    static std::string processLists(const std::string& text);
    static std::string processLineBreaks(const std::string& text);
    static std::string processTableOfContents(const std::string& text);
    static std::string generateTableOfContents(const std::string& text);
    static std::string escapeHtml(const std::string& text);
    static std::string getCSS();
};

#endif // MARKDOWNTOHTML_H