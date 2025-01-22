#include "web_io.h"
#include "common.h"
#include "videoio.h"

extern std::mutex mtxQueueOutput;
extern std::queue<imageout_idx> queueOutput;
extern std::mutex mtxResult;
extern detect_result_group_t result;

void webStreamer(int cpuid) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);

    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        std::cerr << "set thread affinity failed" << std::endl;

    printf("Bind WebStreamer process to CPU %d\n", cpuid);

    boost::asio::io_context io_context;

    StreamServer stream_server(io_context, 8080, 9002); // HTTP port 8080, WebSocket port 9002
    stream_server.start();

    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80}; // Set JPEG quality

    while (1) {
        if (queueOutput.size() > 0) {
            mtxQueueOutput.lock();
            imageout_idx res_pair = queueOutput.front();
            queueOutput.pop();
            mtxQueueOutput.unlock();
            mtxResult.lock();
            result = res_pair.dets;
            mtxResult.unlock();
            draw_image(res_pair.img, res_pair.dets);

            // Encode frame as JPEG
            if (cv::imencode(".jpg", res_pair.img, buffer, params)) {
                stream_server.send_frame(buffer); // Send to clients
            } else {
                std::cerr << "Error: Encoding frame failed!" << std::endl;
            }
            cv::waitKey(33); // 30 FPS
        }
    }
}
