#include <cstdio>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/rtsp/gstrtsptransport.h>

#include <string>
#include <iostream>
#include <cassert>
#include <map>
#include <utility>
#include <vector> 





class GStreamPipeline
{
private:
    typedef std::pair<GstElement*, GstCaps*> gstream_pair;
    typedef std::map<std::string, gstream_pair> gstream_map;

    gstream_map m_pipeline_map;
    GstElement* m_pipeline;
    GMainLoop*  m_loop;

    /**
    @brief creates a gstreame element
    @param element Gstelement to link
    @param caps caps filter can be NULL
    @param child_element child to link
    @param element_name element name
    @param child_element_name child_element_name
    @return true on success false on failure
    */
    bool linkElement(GstElement* element, GstCaps* caps, GstElement* child_element, const std::string& element_name, const std::string& child_element_name)
    {
        if(G_UNLIKELY(!GST_IS_ELEMENT(element)))
        {
            std::cout << "Failed to link: " << element_name << " was the pipeline created?\n";
            return false;
        }
        if(G_UNLIKELY(!GST_IS_ELEMENT(child_element)))
        {
            std::cout << "Failed to link " << element_name << " to " << child_element_name << " was the pipeline created?\n";
            return false;
        }
        if(GST_IS_CAPS(caps))
        {
            if(G_UNLIKELY(!gst_element_link_filtered(element, child_element, caps)))
            {
                gchar* tmp_str = gst_caps_to_string(caps);
                std::cout << "Failed to link " << element_name << " to " << child_element_name <<  " with caps: " << tmp_str << '\n';
                gst_caps_unref(caps);
                g_free(tmp_str);
                return false;
            }
            gst_caps_unref(caps);
            return true;
        }
        else if(G_UNLIKELY(!gst_element_link(element, child_element)))
        {
            std::cout << "Failed to link " << element_name << " to " << child_element_name << '\n'; 
            return false;
        }
        return true;
    }
    /**
    @brief creates a gstream element 
    @param element gstElement plugin name
    @param element_name element name
    @param pipe to attach
    @returns contructed element or NULL on failure
    */
    GstElement* createElement(const std::string& element, const std::string& element_name, GstElement* pipe)
    {   
        GstElement* output;

        if(G_UNLIKELY(pipe == NULL))
            return NULL;
    
        if (G_UNLIKELY (!(output = (gst_element_factory_make (element.c_str(), element_name.c_str()))))) 
            return NULL;
    
        if (G_UNLIKELY (!gst_bin_add (GST_BIN_CAST (pipe), output))) 
        {
            gst_object_unref(G_OBJECT(output));
            return NULL;
        } 
        return output;
    }

public:
    /**
    @brief constructs a pipeline with the provided arguments
    @param pipeline_name name of the pipeline to construct
    @param init_gstream initialize gstreamer in constructor?
    @param create_main_loop create a GMainLoop in constructor? 
    */
    GStreamPipeline(const std::string& pipeline_name, const bool init_gstream = true, const bool create_main_loop = true)
        : m_pipeline_map(), m_pipeline(NULL), m_loop(NULL)
    {
        if(init_gstream)
            gst_init(NULL, NULL);
        if(create_main_loop)
            m_loop = g_main_loop_new(NULL, FALSE);
        
        m_pipeline = gst_pipeline_new(pipeline_name.c_str());
    }
    /**
    @brief does nothing
    */
    ~GStreamPipeline()
    {}

    GStreamPipeline(GStreamPipeline&&) = delete;
    GStreamPipeline(const GStreamPipeline&) = delete;
    /**
    @brief adds an element to the pipeline
    @param element GstElement plugin name
    @param element_name element_name
    @returns true on success false on failure
    */
    bool addElement(const std::string& element, const std::string& element_name)
    {
        GstElement* new_element = createElement(element, element_name, m_pipeline); 
        if(G_UNLIKELY(!GST_IS_ELEMENT(new_element)))
        {
            std::cout << "Failed to add element with name " << element << '\n';
            return false;
        }
        m_pipeline_map[element_name] = {new_element, NULL};
        return true;
    }
    /**
    @brief adds a element to the pipeline
    @param element GstElement plugin name
    @param element_name element_name
    @param element_caps element_caps string
    */
    bool addElement(const std::string& element, const std::string& element_name, const std::string& element_caps)
    {
        GstElement* new_element = createElement(element, element_name, m_pipeline); 
        GstCaps* caps = gst_caps_from_string(element_caps.c_str());
        if(G_UNLIKELY(!GST_IS_ELEMENT(new_element)))
        {
            std::cout << "Failed to add element with name " << element << '\n';
            return false;
        }
        if(G_UNLIKELY(!GST_IS_CAPS(caps)))
        {
            std::cout << "Failed to create element caps with string " << element_caps << '\n';
            return false;
        }
        m_pipeline_map[element_name] = {new_element, caps};
        return false;
    }
    /** 
    @brief Links the elements by list of name
    @param element_name list of nanes
    @return if successfully linked 
    */
    bool linkElementsByName(const std::vector<std::string>& element_names)
    {
        for(auto begin = element_names.begin(); begin != element_names.end(); begin++)
        {
            if(begin + 1 == element_names.end())
                break;
            
            auto found = m_pipeline_map.find(*begin);
            auto child_found = m_pipeline_map.find(*(begin + 1));

            if(found == m_pipeline_map.end())
            {
                std::cout << "Failed to link elements. Could not find element by name: " << *begin << '\n';
                return false;
            }
            if(child_found == m_pipeline_map.end())
            {
                std::cout << "Failed to link elements. Could not find child element by name: " << *begin++;
                return false;
            }

            gstream_pair parent_pair = found->second;
            gstream_pair child_pair = child_found->second;

            if(!linkElement(parent_pair.first, parent_pair.second, child_pair.first, found->first, child_found->first))
                return false;
        }
        return true; 
    }
    /**
    @brief Sets the element signal to supplied callback
    @param element element name
    @param signal_name name of the signal to attach the callback to
    @param callback function callback
    @param user_data user data. can be NULL
    @returns true if the signal was set correctly. Doesn't guaranteed the Element was actually set because the API doesn't have any way to check  
    */
    bool setElementSignal(const std::string& element_name, const std::string& signal_name, GCallback callback, gpointer user_data)
    {
        auto found = m_pipeline_map.find(element_name);
        if(found == m_pipeline_map.end())
            return false;
        
        GstElement* element = std::get<GstElement*>(found->second);
        if(!GST_IS_ELEMENT(element))
            return false;
        
        g_signal_connect(G_OBJECT(element), signal_name.c_str(), G_CALLBACK (callback), user_data);
	return true;
    }
    /**
    @brief Sets the element property to the provided value
    @param element elements name
    @param propery_name the properties name the value will be set to
    @param propery_value the value to set
    @return true if the elements property was set correctly. Doesn't guaranteed the property was actually set because the API doesn't have any way to check  
    */
    template<typename T>
    bool setElementProperty(const std::string& element_name, const std::string& property_name, T property_value)
    {
        auto found = m_pipeline_map.find(element_name);
        if(found == m_pipeline_map.end())
            return false;
        
        GstElement* element = std::get<GstElement*>(found->second);
        if(!GST_IS_ELEMENT(element))
            return false;
        
        g_object_set(G_OBJECT(element), property_name.c_str(), property_value, NULL);
	return true;
    }
    /**
    @brief Returns the GstElement by name
    @param element_name the GstElement name
    @returns See above
    */
    GstElement* getElementByName(const std::string& element_name)
    {
        auto found = m_pipeline_map.find(element_name);
        return found != m_pipeline_map.end() ? found->second.first : NULL;
    }
    /**
    @brief Runs the main loop
    */
    void runMainLoop()
    {
        g_main_loop_run(m_loop);
    }   
    /**
    @brief sets the element state
    @param element_name
    @param element_state 
    @returns if the state was set
    */
    bool setElementState(const std::string& element_name, GstState state)
    {
        auto found = m_pipeline_map.find(element_name);
        if(found == m_pipeline_map.end())
            return false;

        return gst_element_set_state(found->second.first, state) != GST_STATE_CHANGE_FAILURE;
    }
    /**
    @brief sets the pipeline state
    @param state pipeline state to set
    @returns if the state was set
    */
    bool setPipelineState(GstState state)
    {
        return gst_element_set_state(m_pipeline, state) != GST_STATE_CHANGE_FAILURE;
    }
    /**
    @brief attaches the pipeline to a bin
    @returns attached bin, or NULL if failed
    */
    GstElement* attachToBin()
    {
        GstElement* bin = gst_bin_new(NULL);
        if(!gst_bin_add(GST_BIN(bin), m_pipeline))
        {
            gst_object_unref(bin);
            return NULL;
        }
        return bin;
    }
};




static void onPadAdded(GstElement* element, GstPad* pad, GstElement* data)
{
    // Link two Element with named pad
	GstPad *sink_pad = gst_element_get_static_pad (GST_ELEMENT(data), "sink");
	if(gst_pad_is_linked(sink_pad)) 
    {
		g_print("rtspsrc and depay are already linked. Ignoring\n");
		return;
	}
    gst_element_link_pads(element, gst_pad_get_name(pad), GST_ELEMENT(data), "sink");
}


void runSimplePipeline(const std::string& location)
{
    GStreamPipeline pipeline("RTSP_SERVER");
    pipeline.addElement("rtspsrc","rtspsrc");
    pipeline.addElement("rtph264depay", "videodepay");
    pipeline.addElement("h264parse", "h264parse");
    pipeline.addElement("avdec_h264", "videodecode");
    pipeline.addElement("videoscale", "videoscale");
    pipeline.addElement("videorate", "videorate");
    pipeline.addElement("videoconvert", "videoconvert", "video/x-raw, format=(string)I420");
    pipeline.addElement("autovideosink", "videosink");

    pipeline.setElementProperty("rtspsrc", "location", location.c_str());          // location of the stream
    pipeline.setElementProperty("rtspsrc", "protocols", GST_RTSP_LOWER_TRANS_UDP); // set the protocol
    pipeline.setElementProperty("rtspsrc", "latency", 0);                          // latency set to min

    pipeline.linkElementsByName({"videodepay", "h264parse", "videodecode", "videoscale", "videorate", "videoconvert", "videosink"}); 

    pipeline.setElementSignal("rtspsrc", "pad-added", G_CALLBACK(onPadAdded), pipeline.getElementByName("videodepay"));
    
    pipeline.setPipelineState(GST_STATE_PLAYING);
    pipeline.runMainLoop(); 
}





int main(int argc, char* argv[])
{
    assert(argc > 1 && "Provide location of the stream to the program EG: rtsp://192.168.68.52:8554/test");

    runSimplePipeline(argv[1]);
}



