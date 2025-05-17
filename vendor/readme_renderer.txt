SDL_RenderPresent(renderer);

#ifdef USE_SDL_RENDERER
			  	  SDL_RenderPresent(renderer); 
#else
                  SDL_UpdateWindowSurface(window);
#endif


#ifdef USE_SDL_RENDERER
#if SDL_VERSION_ATLEAST(2, 0, 0)
            SMPEG_setdisplay( layer_smpeg_sample, smpeg_update_callback, NULL,  NULL);
#else
            // workaround to set a non-NULL value in the second argument
            SMPEG_setdisplay( layer_smpeg_sample, accumulation_surface, NULL,  NULL);
#endif
#else
            SMPEG_setdisplay( layer_smpeg_sample, screen_surface, NULL,  NULL);
#endif   

