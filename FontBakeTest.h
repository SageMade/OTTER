		LOG_ASSERT(_atlas == nullptr, "Bake has already been called!");
		LOG_ASSERT(_fontInfo.data != nullptr, "Have not loaded a font asset!");

		// Collect all codepoint ranges into a set, so we have a list of unique codepoints
		std::set<int> codePoints;
		uint32_t numCodepoints = 0;
		for (const auto& range : _glyphRanges) {
			for (uint32_t ix = range.x; ix <= range.y; ix++) {
				// skip if the font doesn't have that glyph
				if (!stbtt_FindGlyphIndex(&_fontInfo, ix)) {
					continue;
				}
				numCodepoints += codePoints.emplace(ix).second ? 1 : 0;
			}
		}

		// Allocate our glyph data for the number of unicode character's we're supporting
		_glyphs = new stbtt_packedchar[numCodepoints];

		// Collect unicode ranges, may differ from input ranges!
		std::vector<stbtt_pack_range> ranges;

		// Create the initial data structure, we'll store copies of this as we go
		stbtt_pack_range current;
		current.font_size = _fontSize;
		current.first_unicode_codepoint_in_range = *codePoints.begin();
		current.num_chars = 0;
		current.chardata_for_range = _glyphs;
		current.h_oversample = OVERSAMPLE_X;
		current.v_oversample = OVERSAMPLE_Y;
		current.array_of_unicode_codepoints = (int*)(&*codePoints.begin());

		// Calculate surface area of glyphs as we go
		int totalSurface = 0;
		// We track number of encoded characters, as well as the previous processed codepoint
		// to check for jumps in the range
		uint32_t prevCodePoint = *codePoints.begin() - 1;
		uint32_t encodedChars = 0;

		// Create a temporary array of rects that stb can use as a work space
		std::vector<stbrp_rect> rects;
		rects.resize(codePoints.size() + 1);
		memset(rects.data(), 0, rects.size() * sizeof(stbrp_rect));
		_CrtCheckMemory();

		const float scale = (_fontSize > 0) ? stbtt_ScaleForPixelHeight(&_fontInfo, _fontSize) : stbtt_ScaleForMappingEmToPixels(&_fontInfo, -_fontSize);
		// Iterate over the unique codepoint set (which is sorted!)
		for (uint32_t codepoint : codePoints) {
			// We want to find the size of the glyph, so that we know how big of a texture to make!
			int x, y, x2, y2, w, h;
			// Convert the codepoint to a glyph index so we can look it up in the font
			const int glyph_index_in_font = stbtt_FindGlyphIndex(&_fontInfo, codepoint);
			stbtt_GetGlyphBitmapBoxSubpixel(&_fontInfo, glyph_index_in_font, scale * OVERSAMPLE_X, scale * OVERSAMPLE_Y, 0, 0, &x, &y, &x2, &y2);
			// Calculate our surface area and add it to the total
			totalSurface += (x2 - x + PADDING + OVERSAMPLE_X) * (y2 - y + PADDING + OVERSAMPLE_Y);

			// Store the glyph size
			stbrp_rect& rect = rects[encodedChars];
			rect.w = (x2 - x + PADDING + OVERSAMPLE_X - 1);
			rect.h = (y2 - y + PADDING + OVERSAMPLE_Y - 1);
			rect.x = 0;
			rect.y = 0;

			// We have another character
			encodedChars++;

			// We have a break in the codepoints, start a new range!
			if (codepoint - 1 != prevCodePoint) {
				// Track the end of the current range and store it
				current.num_chars = prevCodePoint - current.first_unicode_codepoint_in_range + 1;
				ranges.push_back(current);

				// Start the next range
				current.first_unicode_codepoint_in_range = codepoint;
				current.chardata_for_range = _glyphs + encodedChars;
				current.array_of_unicode_codepoints = (int*)(&*codePoints.begin()) + encodedChars;
			}

			// Track the previous unicode character
			prevCodePoint = codepoint;
		}

		// We've processed all codepoints, finish the current range and store it
		current.num_chars = prevCodePoint - current.first_unicode_codepoint_in_range + 1;
		ranges.push_back(current);

		_CrtCheckMemory();

		// Get the square root of the total surface area, we can use this to estimate a width to work from
		const int approxWidth = (int)(glm::sqrt((float)totalSurface) + 1);
		// Select a power of 2 texture width (borrowed from ImGui)
		const int texWidth = (approxWidth >= 4096 * 0.7f) ? 
			4096 : 
			(approxWidth >= 2048 * 0.7f) ? 
				2048 : 
				(approxWidth >= 1024 * 0.7f) ? 
					1024 : 
					(approxWidth >= 512 * 0.7f) ?
						512 :
						256;
		// Maximum size of our texture so that stb knows how much vertical space it can use
		const int TEX_MAX = 1024 * 32;

		// Create a packing context and configure it
		stbtt_pack_context context;
		if (!stbtt_PackBegin(&context, NULL, texWidth, TEX_MAX, 0, PADDING, nullptr)) {
			LOG_ERROR("Failed to pack font texture");
			return;
		}
		stbtt_PackSetOversampling(&context, OVERSAMPLE_X, OVERSAMPLE_Y);

		// Pack all our rectangles
		int err = stbrp_pack_rects((stbrp_context*)(context.pack_info), rects.data(), numCodepoints);
		if (!err) {
			LOG_ERROR("Failed to pack font range");
			return;
		}
		_CrtCheckMemory();

		// Next we can iterate over the glyphs and find the max height for the texture
		uint16_t texHeight = 0;
		for (uint32_t ix = 0; ix < rects.size(); ix++) {
			texHeight = glm::max((uint16_t)(rects[ix].y + rects[ix].h), texHeight);
		}

		// Create a texture to store the atlas
		Texture2DDescription desc;
		desc.Width = texWidth;
		desc.Height = texHeight + 1;
		desc.Format = InternalFormat::R8;
		_atlas = std::make_shared<Texture2D>(desc);

		// Allocate memory for the image, and point rect pack at it
		uint8_t* atlasData = new uint8_t[desc.Width * (size_t)desc.Height];
		memset(atlasData, 0, desc.Width * desc.Height);
		context.pixels = atlasData;
		context.height = texHeight;

		_CrtCheckMemory();

		// Render the actual glyphs into the texture
		stbtt_PackFontRangesRenderIntoRects(&context, &_fontInfo, ranges.data(), ranges.size(), rects.data());

		_CrtCheckMemory();

		// Upload data into the image
		_atlas->LoadData(desc.Width, desc.Height, PixelFormat::Red, PixelType::UByte, atlasData);
		delete[] atlasData;


		uint32_t index = 0;
		for (uint32_t codepoint : codePoints) {
			_glyphMap[codepoint] = __CreateGlyph(index++);

			if (codepoint == 0xE000u)
				_defaultGlyph = _glyphMap[codepoint];
		}

		stbtt_PackEnd(&context);