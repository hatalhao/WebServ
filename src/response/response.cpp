#include "../../includes/response.hpp"

Response::Response(const vector<location_b>& locations, const Request& _request) :
        routes(locations) , request(_request) {}

const location_b&	Response::findLocationConfig() {
	const location_b *best = NULL, *root = NULL;
	size_t	bestLen = 0;
	for (vector<location_b>::const_iterator it = this->routes.begin(); it != this->routes.end(); it++) {
		if (it->name == "/")
			root = &(*it);
		if (this->request.getUri().find(it->name) == 0) {
			if(bestLen < it->name.length() || !best) {
				bestLen = it->name.length();
				best = &(*it);
			}
		}
	}
	if (!best)
		best = root;
	return *best;
}

int Response::checkRequirements() {
    const location_b& rules = this->findLocationConfig();
    /**
	 * check for redirections here
	 * 
	 * if (rules.returnCode != 0) {
	 * 	if (rules.returnCode >= 300 && rules.returnCode < 400) {
	 * 		// add Location Header
	 * 	}
	 * 	return rules.returnCode;
	 * }
	**/
	(void)rules;
	if (std::find(rules.methods.begin(),
				rules.methods.end(),
				this->request.getMethod()) == rules.methods.end())
		return (405);
	
    return (0);
}
