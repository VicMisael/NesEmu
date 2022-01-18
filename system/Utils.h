//
// Created by Misael on 12/11/2021.
//

#ifndef NESACOLA_UTILS_H
#define NESACOLA_UTILS_H

#include <stdint.h>

namespace utils {
    /**
     * Checks if the value is between including the lowerBound and upperbound
     * @param lowerbound
     *      the lowerbound of the evaluation.
     * @param upperbound
     *      The upperbound of the evaluation.
     * @param value
     * @return the result of the operation.
     */
    inline bool isBetween(uint16_t lowerbound, uint16_t upperbound, uint16_t &value) {
        return (value>=lowerbound && value<=upperbound);
    }

}
#endif //NESACOLA_UTILS_H
