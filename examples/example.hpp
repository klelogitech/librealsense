#include "example.h"

#include <vector>
#include <sstream>

class texture_buffer
{
    GLuint texture;
    int last_timestamp;
    std::vector<uint8_t> rgb;

    int fps, num_frames, next_time;
public:
    texture_buffer() : texture(), last_timestamp(-1), fps(), num_frames(), next_time(1000) {}

    void upload(rs::device & dev, rs::stream stream)
    {
        assert(dev.is_stream_enabled(stream));

        const rs::intrinsics intrin = dev.get_stream_intrinsics(stream);     
        const int timestamp = dev.get_frame_timestamp(stream);

        // If the frame timestamp has changed since the last time show(...) was called, re-upload the texture
        if(!texture) glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        if(timestamp != last_timestamp)
        {
            const void * data = dev.get_frame_data(stream);
            switch(dev.get_stream_format(stream))
            {
            case rs::format::z16:
                rgb.resize(intrin.width() * intrin.height() * 3);
                make_depth_histogram(rgb.data(), reinterpret_cast<const uint16_t *>(dev.get_frame_data(stream)), intrin.width(), intrin.height());
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
                break;
            case rs::format::yuyv: // Display YUYV by showing the luminance channel and packing chrominance into ignored alpha channel
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data); 
                break;
            case rs::format::rgb8: case rs::format::bgr8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                break;
            case rs::format::rgba8: case rs::format::bgra8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, intrin.width(), intrin.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                break;
            case rs::format::y8:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
                break;
            case rs::format::y16:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
                break;
            }
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            last_timestamp = timestamp;

            ++num_frames;
            if(timestamp >= next_time)
            {
                fps = num_frames;
                num_frames = 0;
                next_time += 1000;
            }
        }    
    }

    void show(rs::device & dev, rs::stream stream, int rx, int ry, int rw, int rh, font & font)
    {
        if(!dev.is_stream_enabled(stream)) return;

        upload(dev, stream);
        
        const rs::intrinsics intrin = dev.get_stream_intrinsics(stream);  
        float h = rh, w = rh * intrin.width() / intrin.height();
        if(w > rw)
        {
            float scale = rw/w;
            w *= scale;
            h *= scale;
        }
        float x0 = rx + (rw - w)/2, x1 = x0 + w, y0 = ry + (rh - h)/2, y1 = y0 + h;

        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2f(x0,y0);
        glTexCoord2f(1,0); glVertex2f(x1,y0);
        glTexCoord2f(1,1); glVertex2f(x1,y1);
        glTexCoord2f(0,1); glVertex2f(x0,y1);
        glEnd();    
        glDisable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);

        std::ostringstream ss; ss << stream << ": " << intrin.width() << " x " << intrin.height() << " " << dev.get_stream_format(stream) << " (" << fps << "/" << dev.get_stream_framerate(stream) << ")";
        ttf_print(&font, rx+8, ry+16, ss.str().c_str());
    }
};