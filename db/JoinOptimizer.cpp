#include <db/JoinOptimizer.h>
#include <db/PlanCache.h>
#include <cfloat>
#include <limits>


using namespace db;

double JoinOptimizer::estimateJoinCost(int card1, int card2, double cost1, double cost2) {
    // TODO pa4.2: some code goes here
    return cost1 + card1 * cost2;

}

int JoinOptimizer::estimateTableJoinCardinality(Predicate::Op joinOp,
                                                const std::string &table1Alias, const std::string &table2Alias,
                                                const std::string &field1PureName, const std::string &field2PureName,
                                                int card1, int card2, bool t1pkey, bool t2pkey,
                                                const std::unordered_map<std::string, TableStats> &stats,
                                                const std::unordered_map<std::string, int> &tableAliasToId) {
    int card = 1;
    int smaller = std::min(card1, card2);
    int larger = std::max(card1, card2);

    switch (joinOp) {
        case Predicate::Op::EQUALS:
            if (t1pkey && t2pkey)
                card = smaller;
            else if (t1pkey)
                card = card2;
            else if (t2pkey)
                card = card1;
            else
                card = larger;
            break;
        case Predicate::Op::LESS_THAN_OR_EQ:
        case Predicate::Op::GREATER_THAN:
            // Assuming that the logic for LESS_THAN_OR_EQ and GREATER_THAN should be similar
            card = static_cast<int>(card1 * card2 * 0.3);
            break;
        default:
            card = card1 * card2;
    }
    return card <= 0 ? 1 : card;
}


std::vector<LogicalJoinNode> JoinOptimizer::orderJoins(std::unordered_map<std::string, TableStats> stats,
                                                       std::unordered_map<std::string, double> filterSelectivities) {
    // TODO pa4.3: some code goes here
    PlanCache planCache;
    std::unordered_set<LogicalJoinNode>* LJNS = nullptr;
    for (size_t i = 0; i < joins.size(); i++) {
        auto setSet = this->enumerateSubsets(this->joins, i + 1);
        for (auto& s : setSet) {
            if (s.size() == joins.size())
                LJNS = &s;

            double bestCost = std::numeric_limits<double>::max();
            CostCard *bestPlan=new CostCard();
            for (const LogicalJoinNode &toRemove : s) {
                CostCard *plan = computeCostAndCardOfSubplan(stats, filterSelectivities, toRemove, s, bestCost, planCache);
                if (plan!= nullptr) {
                    bestPlan = plan;
                    bestCost = plan->cost;
                }
            }
            if (!bestPlan->plan.empty())
                planCache.addPlan(const_cast<std::unordered_set<LogicalJoinNode> &>(s), bestPlan);
        }
    }
    if (LJNS != nullptr) {
        CostCard* finalPlan = planCache.get(*LJNS);
        if (finalPlan != nullptr) {
            // Assuming finalPlan->plan is a std::vector<LogicalJoinNode>
            return finalPlan->plan;
        }
    }
    return {};
}
