#include "../include/handlers/ChatEntryHandler.h"

void ChatEntryHandler::handle(const http::HttpRequest &req, http::HttpResponse *resp)
{
    // 处理 GET 请求，返回 entry.html 文件内容
    // 1. 构建文件路径
    // 2. 检查文件是否存在
    // 3. 读取文件内容
    // 4. 设置响应状态行、关闭连接、内容类型、内容长度、响应体
    // 5. 返回响应
    // 6. 异常处理

    std::string reqFile;
    reqFile.append("../AIApps/ChatServer/resource/entry.html");
    FileUtil fileOperater(reqFile);
    if (!fileOperater.isValid())
    {
        LOG_WARN << reqFile << " not exist";
        fileOperater.resetDefaultFile(); // 404 NOT FOUND
    }

    std::vector<char> buffer(fileOperater.size());
    fileOperater.readFile(buffer); // 读取文件内容到 buffer
    std::string bufStr = std::string(buffer.data(), buffer.size());

    resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
    resp->setCloseConnection(false);
    resp->setContentType("text/html");
    resp->setContentLength(bufStr.size());
    resp->setBody(bufStr);
}
