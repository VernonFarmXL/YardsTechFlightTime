#ifndef _IIRFILTER_H
#define _IIRFILTER_H

// infinite impulse response filter
// alpha can be roughly calculated using 1 - e^-2πfc where fc is the cutoff
// frequency. Alpha should be between 0 and 1 ... the larger the number the
// faster the response
//
//This class is NOT thread-safe.
class IIRFilter
{
public:
    IIRFilter(double initialValue = 0)
    {
        lastVal = initialValue;
        lastValFO = initialValue;
    }

    double processNextStep(double newVal)
    {
        double tempVal = alpha * newVal + (1.0 - alpha) * lastVal;
        lastVal = alpha * tempVal + (1.0 - alpha) * lastValFO;
        lastValFO = tempVal;
        return lastVal;
    }

    double getLastValue() { return lastVal; }

    bool setAlphaValue(double value) { 
        alpha = value;
        return true; 
    }

    double getAlphaValue(){
        return alpha;
    }

private:
    double alpha;
    double lastVal;
    double lastValFO;
};

#endif