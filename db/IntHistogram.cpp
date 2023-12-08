#include <db/IntHistogram.h>
#include <sstream>

using namespace db;

IntHistogram::IntHistogram(int buckets, int min, int max) : min(min), max(max), buckets(buckets), nTuple(0) {
    width = std::ceil(static_cast<double>(max - min + 1) / buckets);
    histogram.resize(buckets, 0);
}

void IntHistogram::addValue(int v) {
    int index = (v == max) ? buckets - 1 : (v - min) / width;
    histogram[index]++;
    nTuple++;
}

double IntHistogram::estimateSelectivity(Predicate::Op op, int v) const {
    if (v < min || v > max) {
        if (op == Predicate::Op::LESS_THAN && v < min) return 0.0;
        if (op == Predicate::Op::GREATER_THAN && v > max) return 0.0;
        return (op == Predicate::Op::NOT_EQUALS) ? 1.0 : 0.0;
    }

    int index = (v == max) ? buckets - 1 : (v - min) / width;
    int left = index * width + min;
    int right = left + width - 1;
    double selectivity = 0.0;

    switch (op) {
        case Predicate::Op::EQUALS:
            return static_cast<double>(histogram[index]) / width / nTuple;

        case Predicate::Op::GREATER_THAN:
            for (int i = index + 1; i < buckets; i++)
                selectivity += histogram[i];
            return (right - v) / static_cast<double>(width) * histogram[index] / nTuple + selectivity / nTuple;

        case Predicate::Op::LESS_THAN:
            for (int i = 0; i < index; i++)
                selectivity += histogram[i];
            return (v - left) / static_cast<double>(width) * histogram[index] / nTuple + selectivity / nTuple;

        case Predicate::Op::LESS_THAN_OR_EQ:
            return estimateSelectivity(Predicate::Op::LESS_THAN, v + 1);

        case Predicate::Op::GREATER_THAN_OR_EQ:
            return estimateSelectivity(Predicate::Op::GREATER_THAN, v - 1);

        case Predicate::Op::LIKE:
            // Assuming avgSelectivity is implemented
            return avgSelectivity();

        case Predicate::Op::NOT_EQUALS:
            return 1.0 - estimateSelectivity(Predicate::Op::EQUALS, v);

        default:
            return 0.0;
    }
}

double IntHistogram::avgSelectivity() const {
    return 1.0;
}

std::string IntHistogram::to_string() const {
    std::stringstream ss;
    ss << "IntHistogram{buckets=" << buckets << ", min=" << min << ", max=" << max << ", histogram=[";
    for (int i = 0; i < buckets; i++) {
        ss << histogram[i];
        if (i < buckets - 1) {
            ss << ", ";
        }
    }
    ss << "]}";
    return ss.str();
}
