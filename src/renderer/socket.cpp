module;
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <variant>

module hammock_renderer;

namespace hammock::renderer {

    // ********** Base socket *********

    BaseSocket::BaseSocket(const std::string &name, SocketInterface iface): name(name), iface(iface) {
    }

    // ********** Image socket ***********

    ImageSocket::ImageSocket(std::string name, const ImageUsage state, const ImageType type, SocketInterface iface): BaseSocket(name, iface),
        state(state), type(type) {
    }

    // ********* Buffer socket **********

    BufferSocket::BufferSocket(std::string name, const BufferUsage state, DescriptorBinding binding): BaseSocket(name, binding),
        state(state) {
    }
}
