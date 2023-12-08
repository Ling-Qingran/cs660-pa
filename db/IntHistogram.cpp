#include <db/IntHistogram.h>
#include <cmath>
#include <sstream>

using namespace db;

IntHistogram::IntHistogram(int buckets, int min, int max) : buckets(buckets), max(max), min(min), totalValues(0){
    width =std::ceil(static_cast<double>(max - min + 1) / buckets);
    bucketCounts.resize(buckets,0);
}

void IntHistogram::addValue(int v) {
    int bucketIndex = getBucketIndex(v);
    bucketCounts[bucketIndex]++;
    totalValues++;
}

double IntHistogram::estimateSelectivity(Predicate::Op op, int v) const {
    int index = getBucketIndex(v);
    if (index < 0 || index >= buckets) {
        return 0.0;
    }

    int bucketCount = bucketCounts[index];
    int leftValue = index * width + min;
    int rightValue = leftValue + width - 1;
    double selectivity = 0.0;

    switch (op) {
        case Predicate::Op::EQUALS:
            if (v < min || v > max) return 0.0;
            return static_cast<double>(bucketCount) / totalValues / width;
        case Predicate::Op::GREATER_THAN: {
            if (v < min)
                return 1.0;
            if (v > max)
                return 0.0;
            int r = 0;
            for (int i = index + 1; i < buckets; i++)
                r += bucketCounts[i];
            return (rightValue - v) / static_cast<double>(width) * static_cast<double>(bucketCounts[index]) / totalValues + static_cast<double>(r) / totalValues;
        }
        case Predicate::Op::LESS_THAN: {
            if (v < min)
                return 0.0;
            if (v > max)
                return 1.0;
            int l = 0;
            for (int i = index - 1; i >= 0; i--)
                l += bucketCounts[i];
            return (v - leftValue) / static_cast<double>(width) * static_cast<double>(bucketCounts[index]) / totalValues + static_cast<double>(l) / totalValues;
        }
        case Predicate::Op::GREATER_THAN_OR_EQ:
            return estimateSelectivity(Predicate::Op::GREATER_THAN, v) + estimateSelectivity(Predicate::Op::EQUALS, v);
        case Predicate::Op::LESS_THAN_OR_EQ:
            return estimateSelectivity(Predicate::Op::LESS_THAN, v) + estimateSelectivity(Predicate::Op::EQUALS, v);
        case Predicate::Op::NOT_EQUALS:
            return 1 - estimateSelectivity(Predicate::Op::EQUALS, v);
        case Predicate::Op::LIKE:
            return avgSelectivity();
    }

    return selectivity;
}

double IntHistogram::avgSelectivity() const {
    return 1.0;
}

std::string IntHistogram::to_string() const {
    std::stringstream ss;
    ss << "Histogram: [";
    for (int count : bucketCounts) {
        ss << count << " ";
    }
    ss << "]";
    return ss.str();
}

int IntHistogram::getBucketIndex(int value) const {
    if (value == max) {
        return buckets - 1;
    }
    return (value - min) / width;
}