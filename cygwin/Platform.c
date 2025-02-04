/*
htop - cygwin/Platform.c
(C) 2014 Hisham H. Muhammad
(C) 2015 Michael McConville
(C) 2015 David C. Hunt
(C) 2017,2018 Guy M. Broome
Released under the GNU GPLv2+, see the COPYING file
in the source distribution for its full text.
*/

#include "cygwin/Platform.h"

#include <math.h>
#include <stdio.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include "ClockMeter.h"
#include "CPUMeter.h"
#include "DateMeter.h"
#include "DateTimeMeter.h"
#include "FileDescriptorMeter.h"
#include "HostnameMeter.h"
#include "LoadAverageMeter.h"
#include "Macros.h"
#include "MemoryMeter.h"
#include "MemorySwapMeter.h"
#include "Meter.h"
#include "Settings.h"
#include "SwapMeter.h"
#include "SysArchMeter.h"
#include "TasksMeter.h"
#include "UptimeMeter.h"
#include "XUtils.h"
#include "cygwin/CygwinMachine.h"


const ScreenDefaults Platform_defaultScreens[] = {
   {
      .name = "Main",
      .columns = "PID USER PRIORITY NICE M_VIRT M_RESIDENT STATE PERCENT_CPU PERCENT_MEM TIME Command",
      .sortKey = "PERCENT_CPU",
   },
};

const unsigned int Platform_numberOfDefaultScreens = ARRAYSIZE(Platform_defaultScreens);

/*
 * See /usr/include/cygwin/signal.h
 */
const SignalItem Platform_signals[] = {
   { .name = " 0 Cancel",    .number =  0 },
   { .name = " 1 SIGHUP",    .number =  1 },
   { .name = " 2 SIGINT",    .number =  2 },
   { .name = " 3 SIGQUIT",   .number =  3 },
   { .name = " 4 SIGILL",    .number =  4 },
   { .name = " 5 SIGTRAP",   .number =  5 },
   { .name = " 6 SIGABRT",   .number =  6 },
   { .name = " 6 SIGIOT",    .number =  6 },
   { .name = " 7 SIGEMT",    .number =  7 },
   { .name = " 8 SIGFPE",    .number =  8 },
   { .name = " 9 SIGKILL",   .number =  9 },
   { .name = "10 SIGBUS",    .number = 10 },
   { .name = "11 SIGSEGV",   .number = 11 },
   { .name = "12 SIGSYS",    .number = 12 },
   { .name = "13 SIGPIPE",   .number = 13 },
   { .name = "14 SIGALRM",   .number = 14 },
   { .name = "15 SIGTERM",   .number = 15 },
   { .name = "16 SIGURG",    .number = 16 },
   { .name = "17 SIGSTOP",   .number = 17 },
   { .name = "18 SIGTSTP",   .number = 18 },
   { .name = "19 SIGCONT",   .number = 19 },
   { .name = "20 SIGCHLD",   .number = 20 },
   { .name = "21 SIGTTIN",   .number = 21 },
   { .name = "22 SIGTTOU",   .number = 22 },
   { .name = "23 SIGIO",     .number = 23 },
   { .name = "24 SIGXCPU",   .number = 24 },
   { .name = "25 SIGXFSZ",   .number = 25 },
   { .name = "26 SIGVTALRM", .number = 26 },
   { .name = "27 SIGPROF",   .number = 27 },
   { .name = "28 SIGWINCH",  .number = 28 },
   { .name = "29 SIGINFO",   .number = 29 },
   { .name = "30 SIGUSR1",   .number = 30 },
   { .name = "31 SIGUSR2",   .number = 31 },
};

const unsigned int Platform_numberOfSignals = ARRAYSIZE(Platform_signals);

const MeterClass* const Platform_meterTypes[] = {
   &CPUMeter_class,
   &ClockMeter_class,
   &DateMeter_class,
   &DateTimeMeter_class,
   &LoadAverageMeter_class,
   &LoadMeter_class,
   &MemoryMeter_class,
   &SwapMeter_class,
   &MemorySwapMeter_class,
   &TasksMeter_class,
   &UptimeMeter_class,
   &BatteryMeter_class,
   &HostnameMeter_class,
   &SysArchMeter_class,
   &AllCPUsMeter_class,
   &AllCPUs2Meter_class,
   &AllCPUs4Meter_class,
   &AllCPUs8Meter_class,
   &LeftCPUsMeter_class,
   &RightCPUsMeter_class,
   &LeftCPUs2Meter_class,
   &RightCPUs2Meter_class,
   &LeftCPUs4Meter_class,
   &RightCPUs4Meter_class,
   &LeftCPUs8Meter_class,
   &RightCPUs8Meter_class,
   &BlankMeter_class,
   &FileDescriptorMeter_class,
   NULL
};

bool Platform_init(void) {
   /* no platform-specific setup needed */
   return true;
}

void Platform_done(void) {
   /* no platform-specific cleanup needed */
}

void Platform_setBindings(Htop_Action* keys) {
   /* no platform-specific key bindings */
   (void) keys;
}

int Platform_getUptime(void) {
   double uptime = 0;
   FILE* fd = fopen(PROCDIR "/uptime", "r");
   if (fd) {
      int n = fscanf(fd, "%64lf", &uptime);
      fclose(fd);
      if (n <= 0) {
         return 0;
      }
   }
   return floor(uptime);
}

void Platform_getLoadAverage(double* one, double* five, double* fifteen) {
   FILE* fd = fopen(PROCDIR "/loadavg", "r");
   if (!fd)
      goto err;

   double scanOne, scanFive, scanFifteen;
   int r = fscanf(fd, "%lf %lf %lf", &scanOne, &scanFive, &scanFifteen);
   fclose(fd);
   if (r != 3)
      goto err;

   *one = scanOne;
   *five = scanFive;
   *fifteen = scanFifteen;
   return;

err:
   *one = NAN;
   *five = NAN;
   *fifteen = NAN;
}

pid_t Platform_getMaxPid(void) {
   // https://github.com/cygwin/cygwin/blob/a9e8e3d1cb8235f513f4d8434509acf287494fcf/winsup/cygwin/local_includes/pinfo.h#L223
   return 65536;
}

double Platform_setCPUValues(Meter* this, unsigned int cpu) {
   const CygwinMachine* chost = (const CygwinMachine *) this->host;
   const Settings* settings = this->host->settings;
   const CPUData* cpuData = &(chost->cpuData[cpu]);
   double total = (double) ( cpuData->totalPeriod == 0 ? 1 : cpuData->totalPeriod);
   double percent;
   double* v = this->values;

   if (!cpuData->online) {
      this->curItems = 0;
      return NAN;
   }

   v[CPU_METER_NICE] = cpuData->nicePeriod / total * 100.0;
   v[CPU_METER_NORMAL] = cpuData->userPeriod / total * 100.0;
   if (settings->detailedCPUTime) {
      v[CPU_METER_KERNEL]  = cpuData->systemPeriod / total * 100.0;
      v[CPU_METER_IRQ]     = cpuData->irqPeriod / total * 100.0;
      v[CPU_METER_SOFTIRQ] = cpuData->softIrqPeriod / total * 100.0;
      this->curItems = 5;

      v[CPU_METER_STEAL]   = cpuData->stealPeriod / total * 100.0;
      v[CPU_METER_GUEST]   = cpuData->guestPeriod / total * 100.0;
      if (settings->accountGuestInCPUMeter) {
         this->curItems = 7;
      }

      v[CPU_METER_IOWAIT]  = cpuData->ioWaitPeriod / total * 100.0;
   } else {
      v[CPU_METER_KERNEL] = cpuData->systemAllPeriod / total * 100.0;
      v[CPU_METER_IRQ] = (cpuData->stealPeriod + cpuData->guestPeriod) / total * 100.0;
      this->curItems = 4;
   }

   percent = sumPositiveValues(v, this->curItems);
   percent = MINIMUM(percent, 100.0);

   if (settings->detailedCPUTime) {
      this->curItems = 8;
   }

   v[CPU_METER_FREQUENCY] = cpuData->frequency;

   v[CPU_METER_TEMPERATURE] = NAN;

   return percent;
}

void Platform_setMemoryValues(Meter* this) {
   const Machine* host = this->host;
   long int usedMem = host->usedMem;
   long int buffersMem = host->buffersMem;
   long int cachedMem = host->cachedMem;
   usedMem -= buffersMem + cachedMem;
   this->total = host->totalMem;
   this->values[MEMORY_METER_USED] = usedMem;
   this->values[MEMORY_METER_BUFFERS] = buffersMem;
   // this->values[MEMORY_METER_SHARED] = "shared memory, like tmpfs and shm"
   // this->values[MEMORY_METER_COMPRESSED] = "compressed memory, like zswap on linux"
   this->values[MEMORY_METER_CACHE] = cachedMem;
   // this->values[MEMORY_METER_AVAILABLE] = "available memory"

}

void Platform_setSwapValues(Meter* this) {
   const Machine* host = this->host;
   this->total = host->totalSwap;
   this->values[SWAP_METER_USED] = host->usedSwap;
   // this->values[SWAP_METER_CACHE] = "pages that are both in swap and RAM, like SwapCached on linux"
   // this->values[SWAP_METER_FRONTSWAP] = "pages that are accounted to swap but stored elsewhere, like frontswap on linux"
}

char* Platform_getProcessEnv(pid_t pid) {
   char procname[128];
   xSnprintf(procname, sizeof(procname), PROCDIR "/%d/environ", pid);
   FILE* fd = fopen(procname, "r");
   if (!fd)
      return NULL;

   char* env = NULL;

   size_t capacity = 0;
   size_t size = 0;
   ssize_t bytes = 0;

   do {
      size += bytes;
      capacity += 4096;
      env = xRealloc(env, capacity);
   } while ((bytes = fread(env + size, 1, capacity - size, fd)) > 0);

   fclose(fd);

   if (bytes < 0) {
      free(env);
      return NULL;
   }

   size += bytes;

   env = xRealloc(env, size + 2);

   env[size] = '\0';
   env[size + 1] = '\0';

   return env;
}

FileLocks_ProcessData* Platform_getProcessLocks(pid_t pid) {
   // TODO
   (void)pid;
   return NULL;
}

void Platform_getFileDescriptors(Meter* this, double* used, double* max) {
   const CygwinMachine* chost = (const CygwinMachine *) this->host;

   *used = chost->openedFDs;
   *max = NAN;  // TODO
}

bool Platform_getDiskIO(DiskIOData* data) {
   // TODO
   (void)data;
   return false;
}

bool Platform_getNetworkIO(NetworkIOData* data) {
   // TODO
   (void)data;
   return false;
}

void Platform_getBattery(double* percent, ACPresence* isOnAC) {
   SYSTEM_POWER_STATUS status;

   if (GetSystemPowerStatus(&status)) {
      if (status.BatteryFlag == BATTERY_FLAG_NO_BATTERY ||
          status.BatteryFlag == BATTERY_FLAG_UNKNOWN)
         goto no_batt;

      if (status.BatteryLifeTime == BATTERY_LIFE_UNKNOWN ||
          status.BatteryFullLifeTime == BATTERY_LIFE_UNKNOWN) {
         if (status.BatteryLifePercent == BATTERY_PERCENTAGE_UNKNOWN)
            *percent = NAN;
         else
            *percent = CLAMP(status.BatteryLifePercent, 0.0, 100.0);
      } else {
         double _percent;

         _percent = status.BatteryLifeTime / status.BatteryFullLifeTime;

         *percent = CLAMP(100 * _percent, 0.0, 100.0);
      }

      if (status.ACLineStatus == AC_LINE_UNKNOWN)
         *isOnAC = AC_ERROR;
      else
         *isOnAC = status.ACLineStatus == AC_LINE_ONLINE;
   } else {
no_batt:
      *percent = NAN;
      *isOnAC = AC_ERROR;
   }
}
