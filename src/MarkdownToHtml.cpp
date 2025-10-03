#include "MarkdownToHtml.h"
#include <sstream>
#include <regex>
#include <algorithm>

std::string MarkdownToHtml::convert(const std::string& markdown) {
    std::string html = markdown;
    
    // Process in order of complexity to avoid conflicts
    html = processCodeBlocks(html);
    html = processHeaders(html);
    html = processTableOfContents(html);
    html = processBold(html);
    html = processItalic(html);
    html = processInlineCode(html);
    html = processLinks(html);
    html = processLists(html);
    html = processLineBreaks(html);
    
    // Wrap in HTML document structure
    std::string fullHtml = R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Docker Manager REST API Documentation</title>
    <style>)" + getCSS() + R"(</style>
</head>
<body>
    <div class="container">
        <header>
            <h1>Docker Manager REST API Documentation</h1>
            <p class="subtitle">Comprehensive API documentation for web developers</p>
        </header>
        <main>)" + html + R"(</main>
        <footer>
            <p>Generated from REST_API_Documentation.md</p>
        </footer>
    </div>
    <script>
        // Add copy-to-clipboard functionality for code blocks
        document.querySelectorAll('pre code').forEach(function(block) {
            const button = document.createElement('button');
            button.className = 'copy-btn';
            button.textContent = 'Copy';
            button.onclick = function() {
                navigator.clipboard.writeText(block.textContent);
                button.textContent = 'Copied!';
                setTimeout(() => button.textContent = 'Copy', 2000);
            };
            block.parentNode.style.position = 'relative';
            block.parentNode.appendChild(button);
        });
        
        // Smooth scrolling for anchor links
        document.querySelectorAll('a[href^="#"]').forEach(anchor => {
            anchor.addEventListener('click', function (e) {
                e.preventDefault();
                const target = document.querySelector(this.getAttribute('href'));
                if (target) {
                    target.scrollIntoView({ behavior: 'smooth' });
                }
            });
        });
    </script>
</body>
</html>)";
    
    return fullHtml;
}

std::string MarkdownToHtml::processHeaders(const std::string& text) {
    std::istringstream iss(text);
    std::string line;
    std::ostringstream oss;
    
    while (std::getline(iss, line)) {
        // Check for headers from h6 to h1 (longest match first)
        std::regex h6_regex(R"(^######\s(.+)$)");
        std::regex h5_regex(R"(^#####\s(.+)$)");
        std::regex h4_regex(R"(^####\s(.+)$)");
        std::regex h3_regex(R"(^###\s(.+)$)");
        std::regex h2_regex(R"(^##\s(.+)$)");
        std::regex h1_regex(R"(^#\s(.+)$)");
        
        std::smatch match;
        
        if (std::regex_match(line, match, h6_regex)) {
            oss << R"(<h6 id=")" << match[1].str() << R"(">)" << match[1].str() << "</h6>\n";
        } else if (std::regex_match(line, match, h5_regex)) {
            oss << R"(<h5 id=")" << match[1].str() << R"(">)" << match[1].str() << "</h5>\n";
        } else if (std::regex_match(line, match, h4_regex)) {
            oss << R"(<h4 id=")" << match[1].str() << R"(">)" << match[1].str() << "</h4>\n";
        } else if (std::regex_match(line, match, h3_regex)) {
            oss << R"(<h3 id=")" << match[1].str() << R"(">)" << match[1].str() << "</h3>\n";
        } else if (std::regex_match(line, match, h2_regex)) {
            oss << R"(<h2 id=")" << match[1].str() << R"(">)" << match[1].str() << "</h2>\n";
        } else if (std::regex_match(line, match, h1_regex)) {
            oss << R"(<h1 id=")" << match[1].str() << R"(">)" << match[1].str() << "</h1>\n";
        } else {
            oss << line << "\n";
        }
    }
    
    return oss.str();
}

std::string MarkdownToHtml::processCodeBlocks(const std::string& text) {
    std::string result = text;
    
    // Process fenced code blocks with language
    std::regex code_block_regex(R"(```(\w+)?\n([\s\S]*?)\n```)");
    result = std::regex_replace(result, code_block_regex, R"(<pre><code class="language-$1">$2</code></pre>)");
    
    return result;
}

std::string MarkdownToHtml::processInlineCode(const std::string& text) {
    std::string result = text;
    
    // Process inline code `code`
    std::regex inline_code_regex(R"(`([^`]+)`)");
    result = std::regex_replace(result, inline_code_regex, R"(<code>$1</code>)");
    
    return result;
}

std::string MarkdownToHtml::processBold(const std::string& text) {
    std::string result = text;
    
    // Process bold **text** and __text__
    std::regex bold_regex1(R"(\*\*([^\*]+)\*\*)");
    result = std::regex_replace(result, bold_regex1, R"(<strong>$1</strong>)");
    
    std::regex bold_regex2(R"(__([^_]+)__)");
    result = std::regex_replace(result, bold_regex2, R"(<strong>$1</strong>)");
    
    return result;
}

std::string MarkdownToHtml::processItalic(const std::string& text) {
    std::string result = text;
    
    // Process italic *text* and _text_
    std::regex italic_regex1(R"(\*([^\*]+)\*)");
    result = std::regex_replace(result, italic_regex1, R"(<em>$1</em>)");
    
    std::regex italic_regex2(R"(_([^_]+)_)");
    result = std::regex_replace(result, italic_regex2, R"(<em>$1</em>)");
    
    return result;
}

std::string MarkdownToHtml::processLinks(const std::string& text) {
    std::string result = text;
    
    // Process links [text](url)
    std::regex link_regex(R"(\[([^\]]+)\]\(([^\)]+)\))");
    result = std::regex_replace(result, link_regex, R"(<a href="$2">$1</a>)");
    
    return result;
}

std::string MarkdownToHtml::processLists(const std::string& text) {
    std::string result = text;
    std::istringstream iss(result);
    std::string line;
    std::ostringstream oss;
    bool inUnorderedList = false;
    bool inOrderedList = false;
    
    while (std::getline(iss, line)) {
        // Check for unordered list items
        std::regex ul_regex(R"(^[-\*\+]\s(.+)$)");
        std::smatch ul_match;
        
        // Check for ordered list items
        std::regex ol_regex(R"(^\d+\.\s(.+)$)");
        std::smatch ol_match;
        
        if (std::regex_match(line, ul_match, ul_regex)) {
            if (!inUnorderedList) {
                if (inOrderedList) {
                    oss << "</ol>\n";
                    inOrderedList = false;
                }
                oss << "<ul>\n";
                inUnorderedList = true;
            }
            oss << "<li>" << ul_match[1].str() << "</li>\n";
        } else if (std::regex_match(line, ol_match, ol_regex)) {
            if (!inOrderedList) {
                if (inUnorderedList) {
                    oss << "</ul>\n";
                    inUnorderedList = false;
                }
                oss << "<ol>\n";
                inOrderedList = true;
            }
            oss << "<li>" << ol_match[1].str() << "</li>\n";
        } else {
            if (inUnorderedList) {
                oss << "</ul>\n";
                inUnorderedList = false;
            }
            if (inOrderedList) {
                oss << "</ol>\n";
                inOrderedList = false;
            }
            oss << line << "\n";
        }
    }
    
    // Close any remaining lists
    if (inUnorderedList) {
        oss << "</ul>\n";
    }
    if (inOrderedList) {
        oss << "</ol>\n";
    }
    
    return oss.str();
}

std::string MarkdownToHtml::processLineBreaks(const std::string& text) {
    std::string result = text;
    
    // Convert double line breaks to paragraphs
    std::regex paragraph_regex(R"(\n\n+)");
    result = std::regex_replace(result, paragraph_regex, "</p>\n<p>");
    
    // Wrap in paragraph tags
    result = "<p>" + result + "</p>";
    
    // Clean up empty paragraphs
    std::regex empty_p_regex(R"(<p>\s*</p>)");
    result = std::regex_replace(result, empty_p_regex, "");
    
    // Single line breaks to <br>
    std::regex br_regex(R"(\n)");
    result = std::regex_replace(result, br_regex, "<br>\n");
    
    return result;
}

std::string MarkdownToHtml::processTableOfContents(const std::string& text) {
    std::string result = text;
    
    // Replace table of contents placeholder
    std::string toc = generateTableOfContents(text);
    std::regex toc_regex(R"(## Table of Contents[\s\S]*?(?=##|\z))");
    result = std::regex_replace(result, toc_regex, "<div class=\"table-of-contents\">\n<h2>Table of Contents</h2>\n" + toc + "\n</div>\n\n");
    
    return result;
}

std::string MarkdownToHtml::generateTableOfContents(const std::string& text) {
    std::ostringstream toc;
    std::istringstream iss(text);
    std::string line;
    
    toc << "<ul class=\"toc-list\">\n";
    
    std::regex header_regex(R"(^(#{1,6})\s(.+)$)");
    std::smatch match;
    
    while (std::getline(iss, line)) {
        if (std::regex_match(line, match, header_regex)) {
            int level = match[1].str().length();
            std::string title = match[2].str();
            
            if (level <= 3) { // Only include h1, h2, h3 in TOC
                std::string indent(level - 1, ' ');
                toc << "  " << indent << "<li><a href=\"#" << title << "\">" << title << "</a></li>\n";
            }
        }
    }
    
    toc << "</ul>";
    
    return toc.str();
}

std::string MarkdownToHtml::escapeHtml(const std::string& text) {
    std::string result = text;
    
    std::regex lt_regex("<");
    result = std::regex_replace(result, lt_regex, "&lt;");
    
    std::regex gt_regex(">");
    result = std::regex_replace(result, gt_regex, "&gt;");
    
    std::regex amp_regex("&");
    result = std::regex_replace(result, amp_regex, "&amp;");
    
    std::regex quote_regex("\"");
    result = std::regex_replace(result, quote_regex, "&quot;");
    
    return result;
}

std::string MarkdownToHtml::getCSS() {
    return R"(
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            background-color: #f8f9fa;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            box-shadow: 0 0 20px rgba(0,0,0,0.1);
            min-height: 100vh;
        }
        
        header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 2rem;
            text-align: center;
        }
        
        header h1 {
            font-size: 2.5rem;
            margin-bottom: 0.5rem;
            font-weight: 700;
        }
        
        .subtitle {
            font-size: 1.1rem;
            opacity: 0.9;
        }
        
        main {
            padding: 2rem;
        }
        
        h1, h2, h3, h4, h5, h6 {
            color: #2c3e50;
            margin: 2rem 0 1rem 0;
            font-weight: 600;
            line-height: 1.3;
        }
        
        h1 { font-size: 2.2rem; border-bottom: 3px solid #3498db; padding-bottom: 0.5rem; }
        h2 { font-size: 1.8rem; border-bottom: 2px solid #e74c3c; padding-bottom: 0.3rem; }
        h3 { font-size: 1.5rem; color: #e67e22; }
        h4 { font-size: 1.3rem; color: #9b59b6; }
        h5 { font-size: 1.1rem; }
        h6 { font-size: 1rem; }
        
        p {
            margin: 1rem 0;
            text-align: justify;
        }
        
        .table-of-contents {
            background: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 8px;
            padding: 1.5rem;
            margin: 2rem 0;
        }
        
        .table-of-contents h2 {
            margin-top: 0;
            border: none;
            color: #495057;
        }
        
        .toc-list {
            list-style: none;
        }
        
        .toc-list li {
            margin: 0.5rem 0;
        }
        
        .toc-list a {
            color: #3498db;
            text-decoration: none;
            padding: 0.25rem 0;
            display: block;
            border-radius: 4px;
            transition: all 0.2s ease;
        }
        
        .toc-list a:hover {
            background: #e3f2fd;
            padding-left: 0.5rem;
            color: #1976d2;
        }
        
        ul, ol {
            margin: 1rem 0;
            padding-left: 2rem;
        }
        
        li {
            margin: 0.5rem 0;
        }
        
        pre {
            background: #2d3748;
            color: #e2e8f0;
            padding: 1.5rem;
            border-radius: 8px;
            overflow-x: auto;
            margin: 1.5rem 0;
            position: relative;
            font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
            font-size: 0.9rem;
            line-height: 1.4;
        }
        
        code {
            background: #f1f3f4;
            padding: 0.2rem 0.4rem;
            border-radius: 4px;
            font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
            font-size: 0.9em;
            color: #e91e63;
        }
        
        pre code {
            background: none;
            padding: 0;
            color: inherit;
            border-radius: 0;
        }
        
        .copy-btn {
            position: absolute;
            top: 0.5rem;
            right: 0.5rem;
            background: #4a5568;
            color: white;
            border: none;
            padding: 0.3rem 0.6rem;
            border-radius: 4px;
            cursor: pointer;
            font-size: 0.8rem;
            transition: background 0.2s ease;
        }
        
        .copy-btn:hover {
            background: #2d3748;
        }
        
        a {
            color: #3498db;
            text-decoration: none;
        }
        
        a:hover {
            color: #2980b9;
            text-decoration: underline;
        }
        
        strong {
            color: #2c3e50;
            font-weight: 600;
        }
        
        em {
            color: #7f8c8d;
            font-style: italic;
        }
        
        hr {
            border: none;
            border-top: 2px solid #ecf0f1;
            margin: 3rem 0;
        }
        
        blockquote {
            border-left: 4px solid #3498db;
            background: #f8f9fa;
            padding: 1rem 1.5rem;
            margin: 1.5rem 0;
            font-style: italic;
        }
        
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 1.5rem 0;
            background: white;
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);
        }
        
        th, td {
            padding: 0.75rem;
            text-align: left;
            border-bottom: 1px solid #dee2e6;
        }
        
        th {
            background: #f8f9fa;
            font-weight: 600;
            color: #495057;
        }
        
        tr:hover {
            background: #f8f9fa;
        }
        
        footer {
            background: #34495e;
            color: white;
            text-align: center;
            padding: 1.5rem;
            margin-top: 3rem;
        }
        
        .endpoint {
            background: #fff;
            border: 1px solid #e1e5e9;
            border-radius: 8px;
            margin: 1.5rem 0;
            overflow: hidden;
        }
        
        .endpoint-header {
            background: #f8f9fa;
            padding: 1rem 1.5rem;
            border-bottom: 1px solid #e1e5e9;
        }
        
        .endpoint-method {
            display: inline-block;
            padding: 0.25rem 0.75rem;
            border-radius: 4px;
            font-weight: 600;
            font-size: 0.85rem;
            margin-right: 1rem;
        }
        
        .method-get { background: #d4edda; color: #155724; }
        .method-post { background: #cce5ff; color: #004085; }
        .method-put { background: #fff3cd; color: #856404; }
        .method-delete { background: #f8d7da; color: #721c24; }
        
        @media (max-width: 768px) {
            .container {
                margin: 0;
                box-shadow: none;
            }
            
            header {
                padding: 1.5rem;
            }
            
            header h1 {
                font-size: 2rem;
            }
            
            main {
                padding: 1rem;
            }
            
            pre {
                padding: 1rem;
                font-size: 0.8rem;
            }
            
            .copy-btn {
                font-size: 0.7rem;
                padding: 0.2rem 0.4rem;
            }
        }
    )";
}