#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <thread>

#include "sampling/ip_cmd_sampler.h"
#include "sampling/procfs_sampler.h"
#include "sampling/sysfs_sampler.h"
#include "termui/bar_chart.h"
#include "termui/display.h"

using namespace bmon;

struct FlowRecord {
    uint64_t rx;
    uint64_t tx;
};

void collect(const std::unique_ptr<sampling::Sampler> &sampler,
             const std::string &iface_name) {
    std::map<std::time_t, FlowRecord> records{};

    sampling::Sample prev_sample = sampler->get_sample(iface_name);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        sampling::Sample sample = sampler->get_sample(iface_name);

        FlowRecord rec{
            sample.rx - prev_sample.rx,
            sample.tx - prev_sample.tx,
        };
        records[sample.ts] = rec;

        prev_sample = sample;

        std::cout << "ts=" << sample.ts << ": rx=" << rec.rx
                  << "B tx=" << rec.tx << "B\n";
    }
}

void visualize(const std::unique_ptr<sampling::Sampler> &sampler,
               const std::string &iface_name, termui::Display &display,
               termui::BarChart &bar_chart) {
    std::vector<uint64_t> rxs{};

    sampling::Sample prev_sample = sampler->get_sample(iface_name);

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        sampling::Sample sample = sampler->get_sample(iface_name);

        FlowRecord rec{
            sample.rx - prev_sample.rx,
            sample.tx - prev_sample.tx,
        };
        rxs.push_back(rec.rx);

        // make sure the vector isn't longer than the width of the display
        if (rxs.size() > display.get_dimensions().width) {
            rxs.erase(rxs.begin());
        }

        prev_sample = sample;

        bar_chart.draw_bars_from_right(rxs);
    }
}

int main_z(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Must pass <iface_name>\n";
        exit(EXIT_FAILURE);
    }

    std::string iface_name{argv[1]};

    std::unique_ptr<sampling::Sampler> ip_sampler{
        new sampling::IpCommandSampler()};
    sampling::Sample sample1 = ip_sampler->get_sample(iface_name);

    std::cout << "ip command sampler (" << iface_name << "):\n";
    std::cout << "RX: " << sample1.rx << "\n";
    std::cout << "TX: " << sample1.tx << "\n";
    std::cout << "ts: " << sample1.ts << "\n";

    std::unique_ptr<sampling::Sampler> proc_sampler{
        new sampling::ProcFsSampler()};
    sampling::Sample sample2 = proc_sampler->get_sample(iface_name);

    std::cout << "\nprocfs command sampler (" << iface_name << "):\n";
    std::cout << "RX: " << sample2.rx << "\n";
    std::cout << "TX: " << sample2.tx << "\n";
    std::cout << "ts: " << sample2.ts << "\n";

    std::unique_ptr<sampling::Sampler> sys_sampler{
        new sampling::SysFsSampler()};
    sampling::Sample sample3 = sys_sampler->get_sample(iface_name);

    std::cout << "\nsysfs command sampler (" << iface_name << "):\n";
    std::cout << "RX: " << sample3.rx << "\n";
    std::cout << "TX: " << sample3.tx << "\n";
    std::cout << "ts: " << sample3.ts << "\n";

    std::cout << "\nrun_forever(" << iface_name << "):\n";
    collect(sys_sampler, iface_name);
}

int main2() {
    termui::Display disp{10};
    disp.initialize();

    termui::BarChart chart{&disp};
    std::vector<uint64_t> values = {1, 2, 3};
    chart.draw_bars_from_right(values);
}

int mainc(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Must pass <iface_name>\n";
        exit(EXIT_FAILURE);
    }

    std::string iface_name{argv[1]};

    std::unique_ptr<sampling::Sampler> sys_sampler{
        new sampling::SysFsSampler()};
    sampling::Sample sample3 = sys_sampler->get_sample(iface_name);

    termui::Display disp{10};
    disp.initialize();

    termui::BarChart chart{&disp};

    visualize(sys_sampler, iface_name, disp, chart);
}




#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

int main() {
    struct winsize size {};

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) < 0) {
        throw std::runtime_error(
            "ioctl() failed when trying to read terminal size");
    }
    auto cols = size.ws_col;
    auto rows = size.ws_row;


    struct termios tm {};
    if (tcgetattr(STDIN_FILENO, &tm) < 0) {
        throw std::runtime_error("tcgetattr() failed");
    }

    // tm.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    tm.c_lflag &= ~(ECHO | ICANON | IEXTEN);
    // tm.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    // tm.c_cflag &= ~(CSIZE | PARENB);
    // tm.c_oflag &= ~(OPOST);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tm) < 0) {
        throw std::runtime_error("tcsetattr() failed");
    }


    std::cout << "cols: " << cols << ", rows: " << rows << "\n";

    auto num_lines = 5;

    for (int y = 0; y < num_lines; ++y) {
        for (int x = 0; x < cols - 0; ++x) {
            fprintf(stdout, "x");
        }
        if (y != num_lines - 1) {
            fprintf(stdout, "\n");
        }
    }

    fprintf(stdout, "\033[%d;%dH", 1, 1);
    fprintf(stdout, "YYY");

    fprintf(stdout, "\033[%d;%dH", rows, cols);
    fprintf(stdout, "T");

    fprintf(stdout, "\033[%d;%dH", num_lines + 1, cols);
    fflush(stdout);

    while (true) {}
}