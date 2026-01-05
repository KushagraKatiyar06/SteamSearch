#include "cosineSimilarity.h"
#include <cmath>
#include <algorithm>

//Constructor
cosineSimilarity::cosineSimilarity(const unordered_map<string, int>& tags) {
    // Create a sorted list of tags to guarantee deterministic indices
    vector<string> sortedTags;
    for (const auto& t : tags) sortedTags.push_back(t.first);
    sort(sortedTags.begin(), sortedTags.end());

    for (size_t i = 0; i < sortedTags.size(); ++i) {
        indexedTags[sortedTags[i]] = static_cast<int>(i);
    }
}

//Getters
unordered_map<string, int> cosineSimilarity::getIndexedTags() {
    return this->indexedTags;
}
unordered_map<string, vector<double>> cosineSimilarity::getGameSignatures() {
    return this->gameSignatures;
}

//Methods
void cosineSimilarity::createGameSignatures(const unordered_map<string, Game>& allData) {
    int numTags = indexedTags.size();

    for (auto const& game : allData) {
        string currentName = game.first;
        Game currentGame = game.second;
        vector<double> gameSignature(numTags, 0.0);

        int totalVotes = 0;
        for (auto tag : currentGame.getTags()) {
            totalVotes += tag.second;
        }

        if (totalVotes == 0) {
            gameSignatures[currentName] = gameSignature;
            continue;
        }

        for (auto tag : currentGame.getTags()) {
            string currentTag = tag.first;
            int currentVotes = tag.second;

            currentTag.erase(remove_if(currentTag.begin(), currentTag.end(), ::isspace), currentTag.end());

            auto it = indexedTags.find(currentTag);
            if (it == indexedTags.end()) continue;

            double score = static_cast<double>(currentVotes) / totalVotes;
            int newIndex = it->second;

            gameSignature[newIndex] += score;
        }

        this->gameSignatures[currentName] = gameSignature;
    }
}

//Now two games have their own signatures which is vector full of the weighted voting each tag has. The dot product
//of both signatures provides the similiarity score after it is normalized using the magnitude of the vector ps: i learned this
//in linear algebra 2 days before writing this algorithm ;P.
double cosineSimilarity::similarity(const string& gameA, const string& gameB) {
    if (gameSignatures.find(gameA) == gameSignatures.end() || gameSignatures.find(gameB) == gameSignatures.end()) {
        return 0.0;
    }

    const vector<double>& gameSignatureA = gameSignatures[gameA];
    const vector<double>& gameSignatureB = gameSignatures[gameB];

    double dotProduct = 0.0;
    double normA = 0.0;
    double normB = 0.0;

    for (int i = 0; i < gameSignatureA.size(); i++) {
        dotProduct += gameSignatureA[i] * gameSignatureB[i];
        normA += pow(gameSignatureA[i], 2);
        normB += pow(gameSignatureB[i], 2);
    }

    normA = sqrt(normA);
    normB = sqrt(normB);

    if (normA == 0 || normB == 0) {
        return 0.0;
    }

    return dotProduct / (normA * normB);
}
