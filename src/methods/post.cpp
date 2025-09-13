#include "../../includes/response.hpp"
#include "../../includes/cgi.hpp"

int	Response::Post() {
	if (this->resourceType == IS_DIR) {
		if (!this->rules->uploadPath.empty())
		{
			if (getResourceType(this->rules->uploadPath.c_str()) != IS_DIR) return (500);
			string	filename = generatefilename();
			std::ofstream outfile((this->rules->uploadPath
				+ (rules->uploadPath[rules->uploadPath.length() - 1] == '/' ? "" : "/")
				+ filename).c_str(), ios::binary);
			if (!outfile.is_open()) return 500;
			outfile.write(this->client->getRequest().getBody().data(), this->client->getRequest().getBody().size());
			outfile.close();
			setHeader("Location", this->client->getRequest().getPath()
					+ (this->client->getRequest().getPath()
						[this->client->getRequest().getPath().length() - 1] == '/' ? "" : "/")
					+ filename);
			setHeader("Content-Length", "0");
			return 201;
		}
		else
			return 500;
	}
	return (setHeader("Allow", "GET"), setHeader("Content-Length", "0"), 405);
}
