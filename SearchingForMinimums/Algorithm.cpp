
#include "Algorithm.h"
#include <cstdlib>


Algorithm::Algorithm(std::string f):
    minList(),
    function(f)
{
}

Algorithm::Algorithm(std::string f,  int divider, int leaving_limit, double precision_optimum, double start_beta, double acceptable_estimation):
    minList(),
    function(f)
{
    BETA_DIVIDER = divider;
    LEAVING_LIMIT = leaving_limit;
    PRECISION_OPTIMUM = precision_optimum;
    START_BETA = start_beta;
    ACCEPTABLE_ESTIMATION = acceptable_estimation;
}

Algorithm::Algorithm(std::string f,  int divider, int leaving_limit, double precision_optimum, double start_beta, double acceptable_estimation, int dimVec):
    minList(),
    function(f, dimVec)
{
    BETA_DIVIDER = divider;
    LEAVING_LIMIT = leaving_limit;
    PRECISION_OPTIMUM = precision_optimum;
    START_BETA = start_beta;
    ACCEPTABLE_ESTIMATION = acceptable_estimation;
}

double derivative(Function& function, VectorN point, VectorN direction, double stepLength ){
    VectorN step = direction.multiply(1/direction.getNorm()).multiply(stepLength);
    VectorN start = point - step;
    VectorN end = point + step;
    return (function.getValue(end) - function.getValue(start))/(2*stepLength);
}

Point Algorithm::searchOneMinimum(VectorN start) {
    
    // VectorN direction_leaving(start.getSize());
    // direction_leaving.setNVal(0,1);
    // start = leaveMaxArea(start, direction_leaving);
    VectorN gradient = function.getGradient(start), prev(start);
    double stepLength = START_BETA;
    unsigned int count = 0;
    while((gradient.getNorm() > PRECISION_OPTIMUM || (start-prev).getNorm()> PRECISION_OPTIMUM) && count < MAX_ITERATIONS ){
        prev = start;
        start = goToMinimum(start, gradient.multiply(-1), stepLength);   // znajduje minimum w kierunku gradientu, minimalizuje funkcje wzgledem beta
        stepLength = (start-prev).getNorm()/BETA_DIVIDER;
        gradient = function.getGradient(start);
        ++count;
    }
    if (count >= MAX_ITERATIONS && !ifMinimum(start)) {// przyjmujemy, ze jesli po MAX_ITERATIONS kroku nie uzyskalismy gradientu zerowego (bliskiego 0) to nie znalezlismy minimum
        return Point();
    }
    return Point(start, function.getValue(start));
}

VectorN Algorithm::goToMinimum(VectorN start, VectorN direction, double stepLength){
    VectorN step = direction.multiply(1/direction.getNorm()).multiply(std::max(stepLength, PRECISION_OPTIMUM));
    VectorN zero(1);
    unsigned int count = 0;
    if(derivative(function, start, direction, PRECISION_DERIVATIVE) > 0 ){ return start; } /* w tym kierunku funkcja rosnie */
    while ((step.getNorm() > PRECISION_OPTIMUM/2 || derivative(function, start+step,direction,PRECISION_DERIVATIVE)<=0) && count < LIMIT_ITERATIONS ){ // dodany warunek pochodnej
        if ( derivative(function, start + step, direction, PRECISION_DERIVATIVE) > 0 ){
            step = step.multiply(0.5);
            // i nie idz kroku do przodu - zrob mniejszy krok lub zakoncz szukanie
        }else {
            start = start + step;
        }
        ++count;
    }
    return start;
}

VectorN Algorithm::goToMaximum(VectorN start, VectorN direction, double stepLength){
    VectorN step = direction.multiply(1/direction.getNorm()).multiply(std::max(stepLength, PRECISION_OPTIMUM));
    VectorN zero(1);
    unsigned int count = 0;
    if (derivative(function, start+step, direction, PRECISION_DERIVATIVE) < 0) {/* zly kierunek - funkcja maleje */ return start; }
    while ((step.getNorm() > PRECISION_OPTIMUM/2 || derivative(function, start+step,direction,PRECISION_DERIVATIVE)>=0) && count < LIMIT_ITERATIONS) { // tutaj dodalem warunek pochodnej
        if ( derivative(function, start+step, direction, PRECISION_DERIVATIVE) < 0 ){
            step = step.multiply(0.5);
            // nie idz kroku do przodu - zrob mniejszy krok lub zakoncz szukanie
        }else {
            start = start + step;
        }
        ++count;
    }
    if(count == LIMIT_ITERATIONS) {
        return VectorN();// zwroc pusty
    }
    return start ;
}

// funkcja idzie w roznych kierunkach starajac sie opuscic maksimum, powinien natomiast minac maksimum (znajdowane bylo w danym kierunku, wiec mijamy je idac w tym samym kierunku)
VectorN Algorithm::leaveMaxArea( VectorN point, VectorN direction ) { // zwraca wektor z wartosciami 0 - gdy dany punkt jest to kierunkowe min
    VectorN result(point), nextPoint(point.getSize()), prevPoint(point.getSize());
    unsigned int n = 0;
    double step = START_BETA, deriv=0;
 
    deriv = derivative(function, result, direction, PRECISION_DERIVATIVE );
    while( deriv > -PRECISION_OPTIMUM && n++<LEAVE_MAXIMUM_AREA_ITERATIONS){
        deriv = derivative(function, result, direction, PRECISION_DERIVATIVE );
        result = result + direction.multiply(step);
    }

    return result;
}

bool Algorithm::ifMinimum(VectorN minCandidate) {
    double prevVal = 0, nextVal = 0, minCandidateVal = function.getValue(minCandidate);
    double step = PRECISION_DERIVATIVE;
    for (int i=0; i<minCandidate.getSize(); ++i){
        VectorN v(minCandidate.getSize());
        v.setNVal(i, 1);
        prevVal = function.getValue(minCandidate - v.multiply(step));
        nextVal = function.getValue(minCandidate + v.multiply(step));
        if( minCandidateVal-prevVal>ACCEPTABLE_ESTIMATION || minCandidateVal-nextVal>ACCEPTABLE_ESTIMATION ){
            return false;
        }
    }
    return true;
}

void Algorithm::leaveMinimum(VectorN start) {
    double stepLength = 0.1;
    VectorN max(function.getVarNum());
    for (int i = 0; i < function.getVarNum(); ++i){
        VectorN direction(function.getVarNum()) ;
        direction.setNVal(i, 1);
        max = goToMaximum(start, direction, stepLength);
        if(!max.isNull()) {
            max = leaveMaxArea(max, direction);
            minList.addMinimumToList(searchOneMinimum(max));
        }
        direction = direction.multiply(-1);
        max = goToMaximum(start, direction, stepLength);
        
        if(!max.isNull()) {
            max = leaveMaxArea(max, direction);
            minList.addMinimumToList(searchOneMinimum(max));
        }
    }
    
}


void Algorithm::searchAllMinimums(VectorN start) {
    unsigned int n = 0;
    VectorN zero(start.getSize()), gradient(start.getSize());
    if(function.getGradient(start)==zero) {
        start = randomStartPoint(start, 10);
        if( function.getGradient(start)==zero) { std::cout << "Prawdopodobnie funkcja stala"; return;}
    }
    Point minimum = searchOneMinimum(start);
    minList.addMinimumToList(minimum);

    while (n < minList.getListMin().size() && n < LEAVING_LIMIT) {
        leaveMinimum(minList.getListMin().at(n).getVectorN());
        ++n;
    }

    // minList.printList();

    // std::cout << "\n Znalezione minimum\n";

}

VectorN Algorithm::randomStartPoint(VectorN point, int rangeLength){
    int lots = 200;
    srand(time(NULL));
    VectorN randomV(point.getSize()), gradient(point.getSize()), zero(point.getSize());
    do {
        for (int i = 0; i < point.getSize(); ++i) {
            randomV.setNVal(i, point.getNVal(i) + rand() % rangeLength);
        }
        gradient = function.getGradient(randomV);
        --lots;
    }while(gradient==zero && lots>0);
    return randomV;
}

Point Algorithm::getFoundOptimum() {
    return minList.getFoundOptimum();
}


