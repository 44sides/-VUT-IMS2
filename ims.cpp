#include "simlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

//  ./ims -t 8 -c 2952 -i 27.3 -ib 13.9 např.

int DIZ_CAPACITY; // 2952
double TRANSPORT_INTERVAL, TRANSPORT_INTERVAL_BIG; // 27.3   &   13.9
int const RADA = 6;
int const ZAVES = 24;
int delic_kos = 0;
int odsazovak_klapky = 0;
int doprava_klonky = 0;
int rohliky = 0;
int doprava_zavesy = 0;
int sklad = 0;

bool firstentry_delic = true;
double lastentry_delic, interval_delic;
double sum_delic = 0, counter_delic = 0;

bool firstentry_hour = true;
int sklad_hour, diff_hour;
int sum_hour = 0, counter_hour = 0;
double time_hour;

Stat dobaObsluhy("Doba obsluhy na lince");

class Transport : public Process {   void Behavior() override;   };

Facility Transport_linka("Transport linka"); 
Facility Predzamis_linka("Predzamis linka");
Facility Delic_linka("Delic linka");
Facility Stroj_linka("Rohlikovy stroj linka");

class Velka_Transport : public Process {
    void Behavior() {
        Wait(4);
        
        double residual_interval_big = TRANSPORT_INTERVAL_BIG - 4;
        Wait(residual_interval_big);

        (new Velka_Transport)->Activate();
    }
};


// kontinualne
class Kontrola_Kynarna_Pec_Pocitacka_KONT : public Process {
    void Behavior() {
        doprava_zavesy += ZAVES;

        Wait(0.5);

        bool vada = false;
        if(Random() <= 0.04)
        {
            doprava_zavesy--;
            vada = true;
        }

        Wait(30);
        Wait(12);
        Wait(0.5); // 30 sec

        if(!vada)
        {
            doprava_zavesy -= ZAVES;
            sklad += ZAVES;
        }
        else
        {
            doprava_zavesy -= ZAVES - 1;
            sklad += ZAVES - 1;
        }

        if(firstentry_hour)
        {
            sklad_hour = ZAVES;
            time_hour = Time;
            firstentry_hour = false;
        }
        else
        {
            if(Time >= (time_hour + 60))
            {
                diff_hour = sklad - sklad_hour;

                sum_hour += diff_hour;
                sklad_hour = sklad;
                time_hour = Time;
                counter_hour++;
            }
        }
    }
};

class Odsazovak : public Process {
    void Behavior() {
        rohliky -= RADA;
        odsazovak_klapky += RADA;

        if(odsazovak_klapky == ZAVES)
        {
            odsazovak_klapky = 0;
            (new Kontrola_Kynarna_Pec_Pocitacka_KONT)->Activate();
        }
    }
};

class Stoceni : public Process {
    void Behavior() {
        Seize(Stroj_linka);

        doprava_klonky -= RADA;
        Wait(0.05); // 3 sec
        rohliky += RADA;

        Release(Stroj_linka);

        Wait(1.5); // 90 sec
        (new Odsazovak)->Activate();
    }
};

// kontinualne
class Draha_Predkynarna_KONT : public Process {
    void Behavior() {
        doprava_klonky += RADA;

        Wait(0.5); // 30 sec
        Wait(6);

        (new Stoceni)->Activate();
    }
};

class Deleni : public Process {
    void Behavior() {
        if(firstentry_delic)
        {
            lastentry_delic = Time;
            firstentry_delic = false;
        }
        else
        {
            interval_delic = Time - lastentry_delic;
            sum_delic += interval_delic;
            lastentry_delic = Time;
            counter_delic++;
        }

        Seize(Delic_linka);
        
        Wait(0.167); // 10 sec

        delic_kos += DIZ_CAPACITY;

        while(delic_kos >= RADA)
        {
            delic_kos -= RADA;
            Wait(0.05); // 3 sec
            (new Draha_Predkynarna_KONT)->Activate();
        }

        Release(Delic_linka);
    }
};

class Predzamis : public Process {
    void Behavior() {
        Seize(Predzamis_linka);

        double davkovani = Uniform(2, 3);
        Wait(davkovani);      (new Transport)->Activate();
        Wait(6);
        Wait(1);

        (new Deleni)->Activate();

        double residual_interval = TRANSPORT_INTERVAL - davkovani - 6 - 1;
        Wait(residual_interval); // residual the interval time

        Release(Predzamis_linka);
    }
};

void Transport::Behavior()
{
    Wait(3.5);

    (new Predzamis)->Activate();
}


int main(int argc, char **argv) {
    if (argc != 9 || !strcmp(argv[1],"--help"))
    {
        fprintf(stderr, "ERROR: Use: -t time -c capacity -i interval -ib interval_big\n");
        exit(1);
    }
    
    double sim_time = atof(argv[2]);
    DIZ_CAPACITY = atoi(argv[4]);
    TRANSPORT_INTERVAL = atof(argv[6]);
    TRANSPORT_INTERVAL_BIG = atof(argv[8]);

    Init(0, 60 * sim_time);
    (new Velka_Transport)->Activate();
    (new Transport)->Activate(Time + 4);
    Run();

    cout << "\nCount sklad: " << sklad << endl;
    cout << "Interval.Mean: " << sum_delic/counter_delic << endl;
    cout << "Vykon: " << sum_hour/counter_hour << endl << endl;
    Transport_linka.Output();
    Predzamis_linka.Output();
    Delic_linka.Output();
    //SIMLIB_statistics.Output();
}